// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <vrpc/mqtt/config.hpp>  // should be top to configure boost::variant limit
#include <vrpc/mqtt/client.hpp>
#include <vrpc/mqtt/sync_client.hpp>
#include <vrpc/mqtt/async_client.hpp>
#include <vrpc/mqtt/connect_flags.hpp>
#include <vrpc/mqtt/connect_return_code.hpp>
#include <vrpc/mqtt/control_packet_type.hpp>
#include <vrpc/mqtt/exception.hpp>
#include <vrpc/mqtt/fixed_header.hpp>
#include <vrpc/mqtt/hexdump.hpp>
#include <vrpc/mqtt/log.hpp>
#include <vrpc/mqtt/publish.hpp>
#include <vrpc/mqtt/setup_log.hpp>
#include <vrpc/mqtt/subscribe_options.hpp>
#include <vrpc/mqtt/remaining_length.hpp>
#include <vrpc/mqtt/reason_code.hpp>
#include <vrpc/mqtt/session_present.hpp>
#include <vrpc/mqtt/utf8encoded_strings.hpp>
#include <vrpc/mqtt/variant.hpp>
#include <vrpc/mqtt/visitor_util.hpp>
#include <vrpc/mqtt/will.hpp>
