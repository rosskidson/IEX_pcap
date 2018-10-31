#include "iex_decoder.h"

#include "Packet.h"
#include "PayloadLayer.h"
#include "PcapFileDevice.h"

bool IEXDecoder::OpenFileForDecoding(const std::string& filename) {
  reader_ptr_.reset(pcpp::IFileReaderDevice::getReader(filename.c_str()));

  // Check the reader was successfully created.
  if (reader_ptr_ == NULL) {
    IEX_LOG("Cannot determine reader for file type\n");
    return false;
  }

  // Open the reader for reading.
  if (!reader_ptr_->open()) {
    IEX_LOG("Cannot open " + filename + " for reading.");
    return false;
  }

  // After initializing the reader, go ahead and decode the first packet already, this should just
  // contain the header.
  // If this doesn't work, it is very unlikely this decoder class will work at all on the given
  // input file.  Therefore do this step upfront, rather than in GetNextMessage.
  if (ParseNextPacket(first_header_) != ReturnCode::Success) {
    IEX_LOG("Failed to parse the first packet.");
    return false;
  }

  // The first packet only contains the header.  Verify this from the header and if so invalidate
  // the packet pointer to force the class to parse the next packet on the first GetNextMessage.
  if (packet_len_ <= first_block_start) {
    packet_ptr_ = nullptr;
  }

  return true;
}

ReturnCode IEXDecoder::ParseNextPacket(IEXTPHeader& header) {
  if (!reader_ptr_) {
    IEX_LOG("The class has not opened a file for reading yet, call OpenFileForDecoding first.");
    return ReturnCode::ClassNotInitialized;
  }

  // Parse the packet.
  pcpp::RawPacket raw_packet;
  if (!reader_ptr_->getNextPacket(raw_packet)) {
    // IEX_LOG("Packet reader returned no more packets to decode.");
    return ReturnCode::EndOfStream;
  };
  parsed_packet_ = pcpp::Packet(&raw_packet);

  // Extract the payload layer. This is used by IEX for message data.
  pcpp::PayloadLayer* payload_layer = parsed_packet_.getLayerOfType<pcpp::PayloadLayer>();
  if (payload_layer == NULL) {
    printf("Couldn't find a generic payload layer for IEX message data.");
    return ReturnCode::FailedParsingPacket;
  }
  packet_ptr_ = payload_layer->getData();
  packet_len_ = payload_layer->getDataLen();
  block_offset_ = first_block_start;

  // Handle header packet.
  bool success = header.Decode(packet_ptr_);
  if (!success) {
    IEX_LOG("Header decode failed.");
    return ReturnCode::FailedDecodingPacket;
  }
  return ReturnCode::Success;
}

ReturnCode IEXDecoder::GetNextMessage(std::unique_ptr<IEXMessageBase>& msg_ptr) {
  if (!reader_ptr_) {
    IEX_LOG("The class has not opened a file for reading yet, " << "call OpenFileForDecoding first.");
    return ReturnCode::ClassNotInitialized;
  }

  // Check if the packet pointer is valid.  If not, the next packet needs to be parsed.
  if (!packet_ptr_) {
    do {
      // Parse the next packet.  This reset block_offset_, packet_len and packet_ptr.
      auto ret_code = ParseNextPacket(last_decoded_header_);
      if (ret_code != ReturnCode::Success) {
        return ret_code;
      }
      // Sometimes the packet is empty. This is a heartbeat from the server every second
      // when there are no new messages.  There is nothing to decode so this loop will skip them.
    } while (last_decoded_header_.payload_len == 0);
  }

  // Get a pointer to the current block.
  const uint8_t* block_ptr = packet_ptr_ + block_offset_;

  // Get the length of current block.
  const int block_len = GetBlockSize(block_ptr);

  // Get the pointer to the data within this block.
  const uint8_t* msg_data_ptr = GetBlockData(block_ptr);

  // Move the block offset to the next block.
  // The +2 is for the two bytes containing the block size not counted in the block length.
  block_offset_ += block_len + 2;

  // If we have gone through the whole packet, reset the pointer.
  if (block_offset_ >= packet_len_) {
    packet_ptr_ = 0;
  }

  msg_ptr = IEXMessageFactory(msg_data_ptr);
  if (!msg_ptr) {
    IEX_LOG("Unknown message type " << PRINTHEX(*msg_data_ptr));
    IEX_LOG("Block len " << block_len);
    return ReturnCode::UnknownMessageType;
  }

  if (!msg_ptr->Decode(msg_data_ptr)) {
    return ReturnCode::FailedDecodingPacket;
  }

  return ReturnCode::Success;
}
