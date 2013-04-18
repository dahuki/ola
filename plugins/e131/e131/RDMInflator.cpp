/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * RDMInflator.cpp
 * The Inflator for the RDM PDUs
 * Copyright (C) 2011 Simon Newton
 */

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include <map>
#include <memory>
#include <string>
#include "ola/Logging.h"
#include "ola/stl/STLUtils.h"
#include "ola/rdm/RDMCommand.h"
#include "plugins/e131/e131/RDMInflator.h"

namespace ola {
namespace plugin {
namespace e131 {

using std::string;

/**
 * Create a new RDM inflator
 */
RDMInflator::RDMInflator()
    : BaseInflator(PDU::ONE_BYTE) {
}


/**
 * Clean up this inflator
 */
RDMInflator::~RDMInflator() {
  STLDeleteValues(&m_rdm_handlers);
}


/**
 * Set a RDMHandler to run for every endpoint.
 * @param handler the callback to invoke when there is rdm data for this
 * universe.
 */
void RDMInflator::SetWildcardRDMHandler(RDMMessageHandler *handler) {
  m_wildcard_handler.reset(handler);
}


/**
 * Set the RDM Handler for an endpoint, ownership of the handler is
 * transferred.
 * @param endpoint the endpoint to use the handler for
 * @param handler the callback to invoke when there is rdm data for this
 * universe.
 * @return true if added, false otherwise
 */
bool RDMInflator::SetRDMHandler(uint16_t endpoint, RDMMessageHandler *handler) {
  if (!handler)
    return false;

  RemoveRDMHandler(endpoint);
  m_rdm_handlers[endpoint] = handler;
  return true;
}


/**
 * Remove the RDM handler for an endpoint
 * @param endpoint the endpoint to remove the handler for.
 * @return true if removed, false if it didn't exist
 */
bool RDMInflator::RemoveRDMHandler(uint16_t endpoint) {
  endpoint_handler_map::iterator iter = m_rdm_handlers.find(endpoint);

  if (iter != m_rdm_handlers.end()) {
    delete iter->second;
    m_rdm_handlers.erase(iter);
    return true;
  }
  return false;
}


/*
 * Decode the RDM 'header', which is 0 bytes in length.
 * @param headers the HeaderSet to add to
 * @param data a pointer to the data
 * @param length length of the data
 * @returns true if successful, false otherwise
 */
bool RDMInflator::DecodeHeader(HeaderSet&,
                               const uint8_t*,
                               unsigned int,
                               unsigned int &bytes_used) {
  bytes_used = 0;
  return true;
}


/*
 * Handle a DMP PDU for E1.33.
 */
bool RDMInflator::HandlePDUData(uint32_t vector,
                                HeaderSet &headers,
                                const uint8_t *data,
                                unsigned int pdu_len) {
  if (vector != VECTOR_RDMNET_DATA) {
    OLA_INFO << "Not a RDM message, vector was " << vector;
    return true;
  }

  string rdm_message(reinterpret_cast<const char*>(&data[0]), pdu_len);

  E133Header e133_header = headers.GetE133Header();

  if (m_wildcard_handler.get()) {
    m_wildcard_handler->Run(&headers.GetTransportHeader(), &e133_header,
                            rdm_message);
  }

  RDMMessageHandler *handler = STLFindOrNull(m_rdm_handlers,
                                             e133_header.Endpoint());

  if (handler) {
    handler->Run(&headers.GetTransportHeader(), &e133_header, rdm_message);
  }
  return true;
}
}  // e131
}  // plugin
}  // ola
