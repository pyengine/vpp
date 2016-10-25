This is a plugin implementing a session tracker, for the L2 bridging path only -
hence the name "L2 sessions", or "l2sess".

This implementation uses the classifier tables for storing the session information.

Moreover, it assumes that whatever table is the topmost table that is configured
as an IP4/IP6 L2 classifier is the one with the mask of TCP/UDP five-tuple - 
these are the only protocols at the moment which track the sessions.
Here and below we call it "session table". There may be more next tables
chained, which we will call below "policy tables" - they can have arbitrary
matches.

This assumption might be lifted in the future versions, however, for the start
it seemed like a good tradeoff, because it allows to avoid the need of 
having any new CLI or API. (Yes, there is a CLI/API for setting/showing binding,
but this is something that is a no-op at the moment, and is reserved/subject
to change in the future).

This implementation of the session tracking is geared primarily for creating
the pinholes for the return traffic in a stateful firewall. For that case,
it is easy to see that it is enough to create the mirrored entries in the
classifier table, if there is a policy applied in that direction.

The idea of this implementation is to have two types of nodes:

- "add" node: the sole reason for the existence of this node is to
add the appropriate sessions to the appropriate session tables.

- "track" node: this node sees all the traffic 
(excluding the packets which create the session), and its purpose is
to track the state of the session for the TCP case, and enforce
the session timeouts for the UDP case.

A classifier lookup on packet that does not match anything in the session
table will cascade onto the policy table, and this is where we can have
a per-match granularity on whether to create the session state or not:
if an entry has hit_next_index being ~0 then the classifier will simply
pass the traffic on to the next feature node.

For the traffic that we want to track statefully, we need to have a
policy entry with hit_next_index equal to that of an appropriate "add" node.
Thus, the first packet will be passed on to the "add" node, which will add
the corresponding entry to the session table, so the next packets
will not hit the policy table but will go straight through without triggering
any new sessions.

The "add" node also adds the mirrored entry into the session table in the other
direction on the same interface.

When creating the session, the "add" node adds the session entry with the hit_next_index
equal to that of a respective "track" node - so that node will see all the traffic
for the session going forward and can take arbitrary actions on the traffic
and the session entry itself.

Both "add" and "track" nodes dispatch to the next node using the same mechanism as
regular L2 features - so from the packet flow perspective they appear spliced between
the L2 classifier and whichever feature node follows.

The rationales for this design were to avoid altering the main packet path, as well
as to allow for arbitrary callers to forward the packets to "add" nodes to add
the sessions.

One can imagine a hypothetical L7 match node which would be triggered by
a coarse grained entry in L2 classifier table, whose hit_next_index in which points
to that node, which in case of match points to the "add" node. 

Timeouts and TCP state tracking.
--------------------------------

The first commit does neither of that. The idea for the subsequent implementation
is to exploit the fact that the session addition is an idempotent operation and
that there are 32 bits of storage in opaque_index field.

TCP tracking at its finest grained tracking needs 8 bits (2xSYN, 2xACK for the SYNs,
2xFINs, 2xACKs for the FINs - though tracking the sequence numbers would not be possible
of course. So maybe this will need to be revisited). Which leaves 24 bits for 
the timekeeping. Realistically, having a 1 second granularity should be fine,
and 24 bits give 16777216 seconds, which is 194 days.

This sounds like a long enough number to be able to implement a "lazy" timeout handling:
If a packet hits an existing session and the l2opaque value is far enough from the low 24 bits
of current unixtime to be bigger than the timeout, then we can wipe the session (and 
its mirror twin in the other table) and drop the packet.

Of course updating this value at present is rather expensive, so should not be done
on a per-packet basis on active flows. A simple idea is to wait with the update on 
an active flow until the difference is big enough to matter - say, at least 10 seconds.

Of course, a future version can evolve to have l2opaque be an index into a session table
which would store arbitrary data, then the limitation of storing only 32 bits will be gone.

The code.
---------

Since there is a lot of boilerplate, for the moment a fair bit of code is created by
preprocessor macros iterating over all of the node names complete with functions,
and taking the actions based on that.

The most important piece of the code is in l2sess.h:

#define _(node_name, node_var, is_out, is_ip6, is_track)
#undef _
#define foreach_l2sess_node \
  _("l2s-input-ip4-add", l2sess_in_ip4_add, 0, 0, 0)  \
  _("l2s-input-ip6-add", l2sess_in_ip6_add, 0, 1, 0)  \
  _("l2s-output-ip4-add", l2sess_out_ip4_add, 1, 0, 0) \
  _("l2s-output-ip6-add", l2sess_out_ip6_add, 1, 1, 0) \
  _("l2s-input-ip4-track", l2sess_in_ip4_track, 0, 0, 1) \
  _("l2s-input-ip6-track", l2sess_in_ip6_track, 0, 1, 1) \
  _("l2s-output-ip4-track",l2sess_out_ip4_track, 1, 0, 1) \
  _("l2s-output-ip6-track", l2sess_out_ip6_track, 1, 1, 1)

By using this macro one can metaprogram the behavior for all of the nodes at once.

For example, there is only one node function, which behaves differently depending on
which of the nodes it is executing on. This is achieved by using the metaprogramming
loop above to set the variables that govern the behavior of the code.
Which brings us to the question of performance.

Performance considerations.
---------------------------

At present, the focus was mainly on getting things right, and easy to maintain/modify.
This is at odds with speed. There are no double loops. So, don't expect it to 
be a speed demon.

