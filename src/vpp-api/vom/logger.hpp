/* -*- C++ -*-; c-basic-offset: 4; indent-tabs-mode: nil */
/*
 * Implementation of logging related utility functions.
 *
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#ifndef __VPP_LOGGER_H__
#define __VPP_LOGGER_H__

#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>

using levels = boost::log::trivial::severity_level;

namespace VPP
{
    /**
     * We use the boost logging library.
     * the expectation is the client configures the sinks as it desires
     */
    typedef boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level> log_t;

    /**
     * Return a log object into which VPP objects can write
     */
    log_t &logger();
};

#endif
