#pragma once

#include "Packet.h"
#include "PcapFileDevice.h"

#include <memory>

#include "iex_messages.h"

/// \enum class ReturnCode
/// \brief An enum for various possible errors when decoding a message.
enum class ReturnCode {
  Success,
  ClassNotInitialized,
  FailedParsingPacket,
  FailedDecodingPacket,
  UnknownMessageType,
  EndOfStream
};

inline std::string ReturnCodeToString(const ReturnCode & code) {
  switch(code) {
    case ReturnCode::Success:
      return "Success";
    case ReturnCode::ClassNotInitialized:
      return "Decoder class not initialized.";
    case ReturnCode::FailedParsingPacket:
      return "Failed parsing packet.";
    case ReturnCode::FailedDecodingPacket:
      return "Failed decoding packet.";
    case ReturnCode::UnknownMessageType:
      return "Unknown message type";
    case ReturnCode::EndOfStream:
      return "End of file stream.";
    default:
      return "Unknown return code.";
  }
}

/// \class IEXDecoder
/// \brief A class for reading and decoding an IEX file stream.
/// \note  All technical information for this implementation was taken from
///        https://iextrading.com/trading/market-data/
///        Use this resource for further information.
class IEXDecoder {
  /// @brief Each packet starts with a header block. This variable describes the length.
  constexpr static size_t first_block_start = 40;

 public:
  IEXDecoder() = default;

  virtual ~IEXDecoder() {
    if (reader_ptr_) {
      reader_ptr_->close();
    };
  }

  /// \brief Open a file for decoding.
  ///
  /// \param filename A string to the relative or full path of the file.
  /// \return True if succeeds, false otherwise.
  bool OpenFileForDecoding(const std::string& filename) WARN_UNUSED;

  /// \brief Get the next message from the stream.
  ///
  /// \param msg_ptr  Output parameter, containing the message if successfully decoded.
  /// \return ReturnCode enum describing success or a specific error code.
  ReturnCode GetNextMessage(std::unique_ptr<IEXMessageBase>& msg_ptr);

  /// \brief Get the first header from the current packet.
  ///
  /// \return A struct populated with the header information.
  inline const IEXTPHeader& GetFirstHeader() { return first_header_; }

  /// \brief Get the last decoded header from the current packet.
  ///
  /// \return A struct populated with the header information.
  inline const IEXTPHeader& GetLastDecodedHeader() { return last_decoded_header_; }

 private:
  /// \brief Get the last decoded header from the current packet.
  ///
  /// \return A struct populated with the header information.
  ReturnCode ParseNextPacket(IEXTPHeader& header) WARN_UNUSED;
  inline uint16_t GetBlockSize(const uint8_t* data_ptr) {
    return *(reinterpret_cast<const uint16_t*>(data_ptr));
  }

  /// \brief Given a pointer pointing to the start of a block, return a pointer pointing to the
  ///        start of the message data.
  ///
  /// \return A pointer pointing to the start of the message data.
  inline const uint8_t* GetBlockData(const uint8_t* data_ptr) { return data_ptr + 2; }

  /// \brief Contains the first header of the current packet being decoded.
  IEXTPHeader first_header_;

  /// \brief Contains the last header decoded of the current packet.
  IEXTPHeader last_decoded_header_;

  /// \brief A pointer of the pcap file reader object.
  std::unique_ptr<pcpp::IFileReaderDevice> reader_ptr_;

  /// \brief The pcpp::Packet needs to stay in context, otherwise the packet memory is deallocated.
  pcpp::Packet parsed_packet_;

  /// \brief A pointer to the start of the current packet. If this is null then there is no packet
  ///        currently being decoded.
  const uint8_t* packet_ptr_;

  /// \brief An offset used to move the message pointer forward through the data.
  size_t block_offset_ = first_block_start;

  /// \brief Length of the currently open packet.
  size_t packet_len_ = 0;
};
