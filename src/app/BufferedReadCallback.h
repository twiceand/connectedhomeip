/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include "lib/core/CHIPTLV.h"
#include "system/SystemPacketBuffer.h"
#include "system/TLVPacketBufferBackingStore.h"
#include <app/AttributePathParams.h>
#include <app/ReadClient.h>
#include <vector>

namespace chip {
namespace app {

/*
 * This is an adapter that intercepts calls that deliver data from the ReadClient,
 * selectively buffers up list chunks in TLV and reconstitutes them into a singular, contiguous TLV array
 * upon completion of delivery of all chunks. This is then delivered to a compliant ReadClient::Callback
 * without any awareness on their part that chunking happened.
 *
 */
class BufferedReadCallback : public ReadClient::Callback
{
public:
    BufferedReadCallback(Callback & callback) : mCallback(callback) {}

private:
    /*
     * Generates the reconsistuted TLV array from the stored individual list elements
     */
    CHIP_ERROR GenerateListTLV(TLV::ScopedBufferTLVReader & reader);

    /*
     * Dispatch any buffered list data if we need to. Buffered data will only be dispatched if:
     *  1. The path provided in aPath is different from the buffered path being tracked internally AND the type of data
     *     in the buffer is list data
     *
     *     OR
     *
     *  2. The path provided in aPath is similar to what is buffered but we've hit the end of the report.
     *
     */
    CHIP_ERROR DispatchBufferedData(const ReadClient * apReadClient, const ConcreteAttributePath & aPath, const StatusIB & aStatus,
                                    bool aEndOfReport = false);

    /*
     * Buffer up list data as they arrive.
     */
    CHIP_ERROR BufferData(const ConcreteDataAttributePath & aPath, TLV::TLVReader * apReader);

private:
    //
    // ReadClient::Callback
    //
    void OnReportBegin(const ReadClient * apReadClient) override;
    void OnReportEnd(const ReadClient * apReadClient) override;
    void OnAttributeData(const ReadClient * apReadClient, const ConcreteDataAttributePath & aPath, TLV::TLVReader * apData,
                         const StatusIB & aStatus) override;
    void OnError(const ReadClient * apReadClient, CHIP_ERROR aError) override { return mCallback.OnError(apReadClient, aError); }
    void OnEventData(const ReadClient * apReadClient, const EventHeader & aEventHeader, TLV::TLVReader * apData,
                     const StatusIB * apStatus) override
    {
        return mCallback.OnEventData(apReadClient, aEventHeader, apData, apStatus);
    }

    void OnDone(ReadClient * apReadClient) override { return mCallback.OnDone(apReadClient); }
    void OnSubscriptionEstablished(const ReadClient * apReadClient) override { mCallback.OnSubscriptionEstablished(apReadClient); }

private:
    /*
     * Given a reader positioned at a list element, allocate a packet buffer, copy the list item where
     * the reader is positioned into that buffer and add it to our buffered list for tracking.
     *
     * This should be called in list index order starting from the lowest index that needs to be buffered.
     *
     */
    CHIP_ERROR BufferListItem(TLV::TLVReader & reader);

    ConcreteDataAttributePath mBufferedPath;
    std::vector<System::PacketBufferHandle> mBufferedList;
    Callback & mCallback;
};

} // namespace app
} // namespace chip
