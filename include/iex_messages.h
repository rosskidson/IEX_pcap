#pragma once

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

// Note: All information for this implementation was taken from the IEX TOPS specification v1.6
//       For further information visit:
//       https://iextrading.com/docs/IEX%20TOPS%20Specification.pdf

// This is simply a convenience function for cout, nothing more.
#define IEX_LOG(msg) std::cout << msg << std::endl;

// This should be used on all functions that return something, so make it easier to use/read.
#define WARN_UNUSED __attribute__((warn_unused_result))

// The following macro and function will take any number, convert it to an int and display it
// as text. This is useful when the reference documentation also uses such hex codes.
#define PRINTHEX(msg) PrintHex(static_cast<int>(msg))

inline std::string PrintHex(const int code) {
  std::stringstream ss;
  ss << "0x" << std::hex << code << std::dec;
  return ss.str();
}

inline char PrintChar(const int code) {
  char ret_val = '0' + code;
  return ret_val;
}

/// \enum MessageType
/// \brief Enum for IEX message types.
enum class MessageType {
  NoData = 0xFF,
  StreamHeader = 0x00,
  SystemEvent = 0x53,
  SecurityDirectory = 0x44,
  SecurityEvent = 0x45,
  TradingStatus = 0x48,
  OperationalHaltStatus = 0x4f,
  ShortSalePriceTestStatus = 0x50,
  QuoteUpdate = 0x51,
  TradeReport = 0x54,
  OfficialPrice = 0x58,
  TradeBreak = 0x42,
  AuctionInformation = 0x41,
  PriceLevelUpdateBuy = 0x38,
  PriceLevelUpdateSell = 0x35
};

/// \brief Convert the message code to a readable string.
///
/// \param The message enum type.
/// \return The type as a string. Output hex code when no matching code found.
inline std::string MessageTypeToString(const MessageType& msg_enum) {
  std::string hex_code = " (" + PRINTHEX(msg_enum) + ")";
  switch (msg_enum) {
    case MessageType::StreamHeader:
      return "Header Message";
    case MessageType::SystemEvent:
      return "SystemEvent" + hex_code;
    case MessageType::SecurityDirectory:
      return "SecurityDirectory" + hex_code;
    case MessageType::TradingStatus:
      return "TradingStatus" + hex_code;
    case MessageType::OperationalHaltStatus:
      return "OperationalHaltStatus" + hex_code;
    case MessageType::ShortSalePriceTestStatus:
      return "ShortSalePriceTestStatus" + hex_code;
    case MessageType::QuoteUpdate:
      return "QuoteUpdate" + hex_code;
    case MessageType::TradeReport:
      return "TradeReport" + hex_code;
    case MessageType::OfficialPrice:
      return "OfficialPrice" + hex_code;
    case MessageType::TradeBreak:
      return "TradeBreak" + hex_code;
    case MessageType::AuctionInformation:
      return "AuctionInformation" + hex_code;
    case MessageType::PriceLevelUpdateBuy:
      return "PriceLevelUpdateBuy" + hex_code;
    case MessageType::PriceLevelUpdateSell:
      return "PriceLevelUpdateSell" + hex_code;
    case MessageType::SecurityEvent:
      return "SecurityEvent" + hex_code;
    default:
      return "Unknown" + hex_code;
  }
}

/// \class IEXMessageBase
/// \brief Base class for all message structs.
class IEXMessageBase {
 public:
  IEXMessageBase() = default;

  virtual ~IEXMessageBase() = default;

  /// \brief Decode the data stream to a message struct.
  ///
  /// \param data_ptr Pointer to the start of the relevant data stream.
  /// \return True if succeeds, false otherwise.
  virtual bool Decode(const uint8_t* data_ptr) WARN_UNUSED = 0;

  /// \brief Output the data to json format.
  std::string OutputToJson() const;

  /// \brief Print contents of message to standard output.
  virtual void Print() const = 0;

  /// \brief Return message type.
  MessageType GetMessageType() const { return message_type; }

  /// \brief Timestamp, nanoseconds since POSIX time UTC.
  uint64_t timestamp;

 protected:
  /// \brief Type of the message.
  MessageType message_type = MessageType::NoData;
};

struct IEXTPHeader : public IEXMessageBase {
  IEXTPHeader() { message_type = MessageType::StreamHeader; }

  /// \brief Decode the data stream to a message struct.
  ///
  /// \param data_ptr Pointer to the start of the relevant data stream.
  /// \return True if succeeds, false otherwise.
  virtual bool Decode(const uint8_t* data_ptr) override WARN_UNUSED;

  /// \brief Print contents of message to standard output.
  virtual void Print() const override;

  /// @brief (0x1) Version of Transport specification
  uint8_t version;

  /// @brief Unique identifier of the higher-layer protocol
  uint16_t protocol_id;

  /// @brief Identifies the stream of bytes/sequenced messages
  uint32_t channel_id;

  /// @brief Integer Identifies the session
  uint32_t session_id;

  /// @brief Short Byte length of the payload
  uint16_t payload_len;

  /// @brief Short Number of messages in the payload
  uint16_t message_count;

  /// @brief Long Byte offset of the data stream
  int64_t stream_offset;

  /// @brief Sequence of the first message in the segment
  int64_t first_msg_sq_num;

  /// @brief Timestamp Send time of segment
  /// @note  Signed integer containing a counter of nanoseconds since POSIX (Epoch) time UTC.
  int64_t send_time;
};

struct SystemEventMessage : public IEXMessageBase {
  enum class Code {
    StartOfMessage = 0x4f,             // 'O'
    StartOfSystemHours = 0x53,         // 'S'
    StartOfRegularMarketHours = 0x52,  // 'R'
    EndOfRegularMarketHours = 0x4d,    // 'M'
    EndOfSystemHours = 0x45,           // 'E'
    EndOfMessages = 0x43               // 'C'
  };

  SystemEventMessage() { message_type = MessageType::SystemEvent; }

  /// \brief Decode the data stream to a message struct.
  ///
  /// \param data_ptr Pointer to the start of the relevant data stream.
  /// \return True if succeeds, false otherwise.
  virtual bool Decode(const uint8_t* data_ptr) override WARN_UNUSED;

  /// \brief Print contents of message to standard output.
  virtual void Print() const override;

  /// \brief System event identifier.
  Code system_event;
};

struct SecurityDirectoryMessage : public IEXMessageBase {
  enum class LULDTier { NotApplicable = 0x0, Tier1NMSStock = 0x1, Tier2NMSStock = 0x2 };

  SecurityDirectoryMessage() { message_type = MessageType::SecurityDirectory; }

  /// \brief Decode the data stream to a message struct.
  ///
  /// \param data_ptr Pointer to the start of the relevant data stream.
  /// \return True if succeeds, false otherwise.
  virtual bool Decode(const uint8_t* data_ptr) override WARN_UNUSED;

  /// \brief Print contents of message to standard output.
  virtual void Print() const override;

  /// \brief See Appendix A for flag values.
  uint8_t flags;

  /// \brief Security identifier.
  std::string symbol;

  /// \brief Integer Number of shares that represent a round lot.
  int round_lot_size;

  /// \brief Corporate action adjusted previous official closing price
  double adjusted_POC_price;

  /// \brief Indicates which Limit Up-Limit Down price band calculation parameter is to be used.
  LULDTier LULD_tier;
};

struct TradingStatusMessage : public IEXMessageBase {
  enum class Status {
    TradingHalted = 0x48,           // 'H'
    TradingHaltReleasedIEX = 0x4f,  // 'O'
    TradingPaused = 0x50,           // 'P'
    Trading = 0x54                  // 'T'
  };

  TradingStatusMessage() { message_type = MessageType::TradingStatus; }

  /// \brief Decode the data stream to a message struct.
  ///
  /// \param data_ptr Pointer to the start of the relevant data stream.
  /// \return True if succeeds, false otherwise.
  virtual bool Decode(const uint8_t* data_ptr) override WARN_UNUSED;

  /// \brief Print contents of message to standard output.
  virtual void Print() const override;

  /// \brief Trading status identifier
  Status trading_status;

  /// \brief Security identifier.
  std::string symbol;

  /// \brief Reason for the trading status change
  std::string reason;
};

struct OperationalHaltStatusMessage : public IEXMessageBase {
  enum class Status {
    IEXOperationalHalt = 0x4f,  // 'O'
    NotHalted = 0x4e            // 'N'
  };

  OperationalHaltStatusMessage() { message_type = MessageType::OperationalHaltStatus; }

  /// \brief Decode the data stream to a message struct.
  ///
  /// \param data_ptr Pointer to the start of the relevant data stream.
  /// \return True if succeeds, false otherwise.
  virtual bool Decode(const uint8_t* data_ptr) override WARN_UNUSED;

  /// \brief Print contents of message to standard output.
  virtual void Print() const override;

  /// \brief Operational halt status identifier.
  Status operational_halt_status;

  /// \brief Security Identifier.
  std::string symbol;
};

struct ShortSalePriceTestStatusMessage : public IEXMessageBase {
  enum class Detail {
    NoPriceTest = 0x20,                     // ' '
    ShortSaleTestIntradayPriceDrop = 0x41,  // 'A'
    ShortSaleTestContinued = 0x43,          // 'C'
    ShortSalePriceDeactivated = 0x44,       // 'D'
    DetailNotAvailable = 0x4e               // 'N'
  };
  ShortSalePriceTestStatusMessage() { message_type = MessageType::ShortSalePriceTestStatus; }

  /// \brief Decode the data stream to a message struct.
  ///
  /// \param data_ptr Pointer to the start of the relevant data stream.
  /// \return True if succeeds, false otherwise.
  virtual bool Decode(const uint8_t* data_ptr) override WARN_UNUSED;

  /// \brief Print contents of message to standard output.
  virtual void Print() const override;

  /// \brief True if Short sale price test in effect, false otherwise.
  bool short_sale_test_in_effect;

  /// \brief Security Identifier.
  std::string symbol;

  /// \brief Detail code.
  Detail detail;
};

struct QuoteUpdateMessage : public IEXMessageBase {
  QuoteUpdateMessage() { message_type = MessageType::QuoteUpdate; }

  /// \brief Decode the data stream to a message struct.
  ///
  /// \param data_ptr Pointer to the start of the relevant data stream.
  /// \return True if succeeds, false otherwise.
  virtual bool Decode(const uint8_t* data_ptr) override WARN_UNUSED;

  /// \brief Print contents of message to standard output.
  virtual void Print() const override;

  /// \brief See Appendix A for flag values.
  uint8_t flags;

  /// \brief Security Identifier.
  std::string symbol;

  /// \brief Aggregate quoted best bid size.
  int bid_size;

  /// \brief Price Best quoted bid price.
  double bid_price;

  /// \brief Integer Aggregate quoted best ask size.
  int ask_size;

  /// \brief Price Best quoted ask price.
  double ask_price;
};

struct TradeReportMessage : public IEXMessageBase {
  TradeReportMessage(const MessageType& msg_type = MessageType::TradeReport) {
    message_type = msg_type;
  }

  /// \brief Decode the data stream to a message struct.
  ///
  /// \param data_ptr Pointer to the start of the relevant data stream.
  /// \return True if succeeds, false otherwise.
  virtual bool Decode(const uint8_t* data_ptr) override WARN_UNUSED;

  /// \brief Print contents of message to standard output.
  virtual void Print() const override;

  /// \brief See Appendix A for flag values.
  uint8_t flags;

  /// \brief Security Identifier.
  std::string symbol;

  /// \brief Trade volume.
  int size;

  /// \brief Trade price.
  double price;

  /// \brief IEX Generated Identifier. Trade ID is also referenced in the Trade Break Message.
  int trade_id;
};

struct OfficialPriceMessage : public IEXMessageBase {
  enum class PriceType {
    OpeningPrice = 0x51,  // 'Q'
    ClosingPrice = 0x4d   // 'M'
  };

  OfficialPriceMessage() { message_type = MessageType::OfficialPrice; }

  /// \brief Decode the data stream to a message struct.
  ///
  /// \param data_ptr Pointer to the start of the relevant data stream.
  /// \return True if succeeds, false otherwise.
  virtual bool Decode(const uint8_t* data_ptr) override WARN_UNUSED;

  /// \brief Print contents of message to standard output.
  virtual void Print() const override;

  /// \brief Price type identifier.
  PriceType price_type;

  /// \brief Security Identifier.
  std::string symbol;

  /// \brief Official opening or closing price, as specified.
  double price;
};

struct AuctionInformationMessage : public IEXMessageBase {
  enum class AuctionType {
    OpeningAuction = 0x4f,    // 'O'
    ClosingAuction = 0x43,    // 'C'
    IPOAuction = 0x49,        // 'I'
    HaltAuction = 0x48,       // 'H'
    VolatilityAuction = 0x56  // 'V'
  };

  enum class ImbalanceSide {
    BuySideImbalance = 0x42,   // 'B'
    SellSideImbalance = 0x53,  // 'S'
    NoImbalance = 0x4e         // 'N'
  };

  AuctionInformationMessage() { message_type = MessageType::AuctionInformation; }

  /// \brief Decode the data stream to a message struct.
  ///
  /// \param data_ptr Pointer to the start of the relevant data stream.
  /// \return True if succeeds, false otherwise.
  virtual bool Decode(const uint8_t* data_ptr) override WARN_UNUSED;

  /// \brief Print contents of message to standard output.
  virtual void Print() const override;

  /// \brief See Appendix A for flag values.
  AuctionType auction_type;

  /// \brief Security Identifier.
  std::string symbol;

  /// \brief Number of shares paired at the Reference Price using orders on the Auction Book.
  int paired_shares;

  /// \brief Clearing price at or within the Reference Price range using orders on the Auction Book.
  double reference_price;

  /// \brief Price Clearing price using Eligible Auction Orders.
  double indicative_clearing_price;

  /// \brief Number of unpaired shares at the Reference Price using orders on the Auction Book.
  int imbalance_shares;

  /// \brief Side of the unpaired shares at the Reference Price using orders on the Auction Book.
  ImbalanceSide imbalance_side;

  /// \brief Number of extensions an auction received.
  int extension_number;

  /// \brief Projected time of the auction match. Time is seconds since POSIX, UTC.
  int scheduled_auction_time;

  /// \brief Clearing price using orders on the Auction Book.
  double auction_book_clearing_price;

  /// \brief Reference price used for the auction collar, if any.
  double collar_reference_price;

  /// \brief Lower threshold price of the auction collar, if any.
  double lower_auction_collar;

  /// \brief Upper threshold price of the auction collar, if any.
  double upper_auction_collar;
};

struct PriceLevelUpdateMessage : public IEXMessageBase {
  PriceLevelUpdateMessage(const MessageType& msg_type) { message_type = msg_type; }

  /// \brief Decode the data stream to a message struct.
  ///
  /// \param data_ptr Pointer to the start of the relevant data stream.
  /// \return True if succeeds, false otherwise.
  virtual bool Decode(const uint8_t* data_ptr) override WARN_UNUSED;

  /// \brief Print contents of message to standard output.
  virtual void Print() const override;

  /// \brief See Appendix A for flag values.
  uint8_t flags;

  /// \brief Security Identifier.
  std::string symbol;

  /// \brief Aggregate quoted size.
  int size;

  /// \brief Price level to add/update in the IEX Order Book.
  double price;
};

struct SecurityEventMessage : public IEXMessageBase {
  enum class SecurityMessageType {
    OpeningProcessComplete = 0x4f,    // 'O'
    ClosingProcessComplete = 0x43,    // 'C'
  };

  SecurityEventMessage(const MessageType& msg_type) { message_type = msg_type; }

  /// \brief Decode the data stream to a message struct.
  ///
  /// \param data_ptr Pointer to the start of the relevant data stream.
  /// \return True if succeeds, false otherwise.
  virtual bool Decode(const uint8_t* data_ptr) override WARN_UNUSED;

  /// \brief Print contents of message to standard output.
  virtual void Print() const override;

  /// \brief Security event.
  SecurityMessageType security_event;

  /// \brief Security Identifier.
  std::string symbol;
};


std::unique_ptr<IEXMessageBase> IEXMessageFactory(const uint8_t* msg_data_ptr);
