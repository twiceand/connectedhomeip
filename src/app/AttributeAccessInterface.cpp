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

#include <app/AttributeAccessInterface.h>

namespace chip {
namespace app {

CHIP_ERROR AttributeReportBuilder::PrepareAttribute(AttributeReportIBs::Builder & aAttributeReportIBsBuilder,
                                                    const ConcreteDataAttributePath & aPath, DataVersion aDataVersion)
{
    mAttributeReportIBBuilder   = aAttributeReportIBsBuilder.CreateAttributeReport();
    mAttributeDataIBBuilder     = mAttributeReportIBBuilder.CreateAttributeData();
    auto attributePathIBBuilder = mAttributeDataIBBuilder.CreatePath();

    ReturnErrorOnFailure(mAttributeReportIBBuilder.GetError());
    ReturnErrorOnFailure(mAttributeDataIBBuilder.GetError());

    attributePathIBBuilder.Endpoint(aPath.mEndpointId).Cluster(aPath.mClusterId).Attribute(aPath.mAttributeId);

    // Encode the list index field if we are encoding a single element in an list.
    if (aPath.mListOp == ConcreteDataAttributePath::ListOperation::AppendItem)
    {
        // The Null ListIndex (append operation)
        attributePathIBBuilder.ListIndex(DataModel::Nullable<ListIndex>());
    }

    ReturnErrorOnFailure(attributePathIBBuilder.EndOfAttributePathIB().GetError());
    mAttributeDataIBBuilder.DataVersion(aDataVersion);

    return mAttributeDataIBBuilder.GetError();
}

CHIP_ERROR AttributeReportBuilder::FinishAttribute()
{
    ReturnErrorOnFailure(mAttributeDataIBBuilder.EndOfAttributeDataIB().GetError());
    return mAttributeReportIBBuilder.EndOfAttributeReportIB().GetError();
}

CHIP_ERROR AttributeValueEncoder::EncodeEmptyList()
{
    if (mCurrentEncodingListIndex == kInvalidListIndex)
    {
        if (mEncodeState.mCurrentEncodingListIndex == kInvalidListIndex)
        {
            // Clear mAllowPartialData flag here since this encode procedure is not atomic.
            // The most common error in this function is CHIP_ERROR_NO_MEMORY / CHIP_ERROR_BUFFER_TOO_SMALL, just revert and try
            // next time is ok.
            mEncodeState.mAllowPartialData = false;
            // Spec 10.5.4.3.1, 10.5.4.6 (Replace a list w/ Multiple IBs)
            // Put an empty array before encoding the first array element for list chunking.
            AttributeReportBuilder builder;

            ReturnErrorOnFailure(builder.PrepareAttribute(
                mAttributeReportIBsBuilder, ConcreteDataAttributePath(mPath.mEndpointId, mPath.mClusterId, mPath.mAttributeId),
                mDataVersion));
            ReturnErrorOnFailure(builder.Encode(DataModel::List<uint8_t>()));

            ReturnErrorOnFailure(builder.FinishAttribute());
            mEncodeState.mCurrentEncodingListIndex = 0;
        }
        mCurrentEncodingListIndex = 0;
    }

    return CHIP_NO_ERROR;
}

} // namespace app
} // namespace chip
