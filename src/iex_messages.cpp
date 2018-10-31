#include "iex_messages.h"
#include <algorithm>

/// \brief Templated function for dereferencing and casting a uint8_t pointer to a desired type.
///
/// \param data_ptr  Pointer to the data.
/// \param offset    An offset to first apply to the pointer before dereferencing.
/// \return template type T, the numeric data being requested.
template <typename T>
T GetNumeric(const uint8_t* data_ptr, const int offset) {
  return *(reinterpret_cast<const T*>(&data_ptr[offset]));
}

/// \brief Similar to GetNumeric, however specialized for price data.
///
/// \param data_ptr  Pointer to the data.
/// \param offset    An offset to first apply to the pointer before dereferencing.
/// \return The price in dollars, returned as a double.
double GetPrice(const uint8_t* data_ptr, const int offset) {
  return *(reinterpret_cast<const int64_t*>(&data_ptr[offset])) / 10000.0;
}

/// \brief Similar to GetNumeric, however specialized for string data.
///
/// \param data_ptr  Pointer to the data.
/// \param offset    An offset to first apply to the pointer before dereferencing.
/// \param length    Expected length of the string.
/// \return The string data as an std::string
std::string GetString(const uint8_t* data_ptr, const int offset, const int length) {
  std::string ret_val = std::string((reinterpret_cast<const char*>(&data_ptr[offset])), length);
  // Remove whitespace.
  ret_val.erase(
      std::find_if(ret_val.rbegin(), ret_val.rend(), [](int ch) { return !std::isspace(ch); })
          .base(),
      ret_val.end());
  return ret_val;
}

/// \brief Validate the timestamp using a sensible range.
/// \note  Lower limit is 2018-10-25, when IEX opened for trading, upper limit is 2100.
///
/// \param timestamp  Input timestamp to validate.
/// \return bool True if timestamp is valid, false otherwise.
bool ValidateTimestamp(const int64_t timestamp) {
  return (timestamp > 1382659200000000000) && (timestamp < 4102444800000000000);
}

std::string IEXMessageBase::OutputToJson() const { return "Not implemented"; }

bool IEXTPHeader::Decode(const uint8_t* data_ptr) {
  version = GetNumeric<uint8_t>(data_ptr, 0);
  protocol_id = GetNumeric<uint16_t>(data_ptr, 2);
  channel_id = GetNumeric<uint32_t>(data_ptr, 4);
  session_id = GetNumeric<uint32_t>(data_ptr, 8);
  payload_len = GetNumeric<uint16_t>(data_ptr, 12);
  message_count = GetNumeric<uint16_t>(data_ptr, 14);
  stream_offset = GetNumeric<uint64_t>(data_ptr, 16);
  first_msg_sq_num = GetNumeric<uint64_t>(data_ptr, 24);
  send_time = GetNumeric<uint64_t>(data_ptr, 32);

  if (version != 1) {
    IEX_LOG("Error: The version of the transport specification has changed. Decoding may not work");
    return false;
  } else {
    return true;
  }
}

void IEXTPHeader::Print() const {
  IEX_LOG("ver               : " << int(version));
  IEX_LOG("id                : " << protocol_id);
  IEX_LOG("channel_id        : " << channel_id);
  IEX_LOG("session_id        : " << session_id);
  IEX_LOG("payload_len       : " << payload_len);
  IEX_LOG("message count     : " << message_count);
  IEX_LOG("stream offset     : " << stream_offset);
  IEX_LOG("first message     : " << first_msg_sq_num);
  IEX_LOG("send time         : " << send_time << std::endl);
}

bool SystemEventMessage::Decode(const uint8_t* data_ptr) {
  system_event = static_cast<SystemEventMessage::Code>(GetNumeric<uint8_t>(data_ptr, 1));
  timestamp = GetNumeric<uint64_t>(data_ptr, 2);

  return ValidateTimestamp(timestamp);
}

void SystemEventMessage::Print() const {
  IEX_LOG("Message type      : " << MessageTypeToString(message_type));
  IEX_LOG("Timestamp         : " << timestamp);
  IEX_LOG("System event      : " << static_cast<char>(system_event));
}

bool SecurityDirectoryMessage::Decode(const uint8_t* data_ptr) {
  flags = GetNumeric<uint8_t>(data_ptr, 1);
  timestamp = GetNumeric<uint64_t>(data_ptr, 2);
  symbol = GetString(data_ptr, 10, 8);
  round_lot_size = GetNumeric<uint32_t>(data_ptr, 18);
  adjusted_POC_price = GetPrice(data_ptr, 22);
  LULD_tier = static_cast<LULDTier>(GetNumeric<uint8_t>(data_ptr, 30));

  return ValidateTimestamp(timestamp);
}

void SecurityDirectoryMessage::Print() const {
  IEX_LOG("Message type      : " << MessageTypeToString(message_type));
  IEX_LOG("Timestamp         : " << timestamp);
  IEX_LOG("Symbol            : " << symbol);
  IEX_LOG("Flag              : " << PRINTHEX(flags));
  IEX_LOG("Round lot size    : " << round_lot_size);
  IEX_LOG("Adjust POC price  : " << adjusted_POC_price);
  IEX_LOG("LULD Tier         : " << static_cast<int>(LULD_tier));
}

bool TradingStatusMessage::Decode(const uint8_t* data_ptr) {
  trading_status = static_cast<TradingStatusMessage::Status>(GetNumeric<uint8_t>(data_ptr, 1));
  timestamp = GetNumeric<uint64_t>(data_ptr, 2);
  symbol = GetString(data_ptr, 10, 8);
  reason = GetString(data_ptr, 18, 4);

  return ValidateTimestamp(timestamp);
}

void TradingStatusMessage::Print() const {
  IEX_LOG("Message type      : " << MessageTypeToString(message_type));
  IEX_LOG("Timestamp         : " << timestamp);
  IEX_LOG("Symbol            : " << symbol);
  IEX_LOG("Trading status    : " << static_cast<char>(trading_status));
  IEX_LOG("Reason            : " << reason);
}

bool OperationalHaltStatusMessage::Decode(const uint8_t* data_ptr) {
  operational_halt_status =
      static_cast<OperationalHaltStatusMessage::Status>(GetNumeric<uint8_t>(data_ptr, 1));
  timestamp = GetNumeric<uint64_t>(data_ptr, 2);
  symbol = GetString(data_ptr, 10, 8);

  return ValidateTimestamp(timestamp);
}

void OperationalHaltStatusMessage::Print() const {
  IEX_LOG("Message type      : " << MessageTypeToString(message_type));
  IEX_LOG("Timestamp         : " << timestamp);
  IEX_LOG("Symbol            : " << symbol);
  IEX_LOG("Operational halt  : " << static_cast<char>(operational_halt_status));
}

bool ShortSalePriceTestStatusMessage::Decode(const uint8_t* data_ptr) {
  short_sale_test_in_effect = static_cast<bool>(GetNumeric<uint8_t>(data_ptr, 1));
  timestamp = GetNumeric<uint64_t>(data_ptr, 2);
  symbol = GetString(data_ptr, 10, 8);
  detail = static_cast<Detail>(GetNumeric<uint8_t>(data_ptr, 18));

  return ValidateTimestamp(timestamp);
}

void ShortSalePriceTestStatusMessage::Print() const {
  IEX_LOG("Message type      : " << MessageTypeToString(message_type));
  IEX_LOG("Timestamp         : " << timestamp);
  IEX_LOG("Symbol            : " << symbol);
  IEX_LOG("In effect         : " << short_sale_test_in_effect);
  IEX_LOG("Detail            : " << static_cast<char>(detail));
}

bool QuoteUpdateMessage::Decode(const uint8_t* data_ptr) {
  flags = GetNumeric<uint8_t>(data_ptr, 1);
  timestamp = GetNumeric<uint64_t>(data_ptr, 2);
  symbol = GetString(data_ptr, 10, 8);
  bid_size = GetNumeric<uint32_t>(data_ptr, 18);
  bid_price = GetPrice(data_ptr, 22);
  ask_size = GetNumeric<uint32_t>(data_ptr, 38);
  ask_price = GetPrice(data_ptr, 30);

  return ValidateTimestamp(timestamp);
}

void QuoteUpdateMessage::Print() const {
  IEX_LOG("Message type      : " << MessageTypeToString(message_type));
  IEX_LOG("Timestamp         : " << timestamp);
  IEX_LOG("Symbol            : " << symbol);
  IEX_LOG("Flag              : " << PRINTHEX(flags));
  IEX_LOG("Bid size          : " << bid_size);
  IEX_LOG("Bid price         : " << bid_price);
  IEX_LOG("Ask size          : " << ask_size);
  IEX_LOG("Ask price         : " << ask_price);
}

bool TradeReportMessage::Decode(const uint8_t* data_ptr) {
  flags = GetNumeric<uint8_t>(data_ptr, 1);
  timestamp = GetNumeric<uint64_t>(data_ptr, 2);
  symbol = GetString(data_ptr, 10, 8);
  size = GetNumeric<uint32_t>(data_ptr, 18);
  price = GetPrice(data_ptr, 22);
  trade_id = GetNumeric<uint64_t>(data_ptr, 30);

  return ValidateTimestamp(timestamp);
}

void TradeReportMessage::Print() const {
  IEX_LOG("Message type      : " << MessageTypeToString(message_type));
  IEX_LOG("Timestamp         : " << timestamp);
  IEX_LOG("Symbol            : " << symbol);
  IEX_LOG("Flag              : " << PRINTHEX(flags));
  IEX_LOG("Size              : " << size);
  IEX_LOG("Price             : " << price);
  IEX_LOG("Trade id          : " << trade_id);
}

bool OfficialPriceMessage::Decode(const uint8_t* data_ptr) {
  price_type = static_cast<PriceType>(GetNumeric<uint8_t>(data_ptr, 1));
  timestamp = GetNumeric<uint64_t>(data_ptr, 2);
  symbol = GetString(data_ptr, 10, 8);
  price = GetPrice(data_ptr, 18);

  return ValidateTimestamp(timestamp);
}

void OfficialPriceMessage::Print() const {
  IEX_LOG("Message type      : " << MessageTypeToString(message_type));
  IEX_LOG("Timestamp         : " << timestamp);
  IEX_LOG("Symbol            : " << symbol);
  IEX_LOG("Price type        : " << static_cast<char>(price_type));
  IEX_LOG("Official price    : " << price);
}

bool AuctionInformationMessage::Decode(const uint8_t* data_ptr) {
  auction_type = static_cast<AuctionType>(GetNumeric<uint8_t>(data_ptr, 1));
  timestamp = GetNumeric<uint64_t>(data_ptr, 2);
  symbol = GetString(data_ptr, 10, 8);
  paired_shares = GetNumeric<uint32_t>(data_ptr, 18);
  reference_price = GetPrice(data_ptr, 22);
  indicative_clearing_price = GetPrice(data_ptr, 30);
  imbalance_shares = GetNumeric<uint32_t>(data_ptr, 38);
  imbalance_side =
      static_cast<AuctionInformationMessage::ImbalanceSide>(GetNumeric<uint8_t>(data_ptr, 42));
  extension_number = GetNumeric<uint8_t>(data_ptr, 43);
  scheduled_auction_time = GetNumeric<uint32_t>(data_ptr, 44);
  auction_book_clearing_price = GetPrice(data_ptr, 48);
  collar_reference_price = GetPrice(data_ptr, 56);
  lower_auction_collar = GetPrice(data_ptr, 64);
  upper_auction_collar = GetPrice(data_ptr, 72);

  return ValidateTimestamp(timestamp);
}

void AuctionInformationMessage::Print() const {
  IEX_LOG("Message type      : " << MessageTypeToString(message_type));
  IEX_LOG("Timestamp         : " << timestamp);
  IEX_LOG("Symbol            : " << symbol);
  IEX_LOG("Auction type      : " << static_cast<char>(auction_type));
  IEX_LOG("Paired shares     : " << paired_shares);
  IEX_LOG("Reference price   : " << reference_price);
  IEX_LOG("Indicative clear  : " << indicative_clearing_price);
  IEX_LOG("Imbalance shares  : " << imbalance_shares);
  IEX_LOG("Imbalance side    : " << static_cast<char>(imbalance_side));
  IEX_LOG("Extension number  : " << extension_number);
  IEX_LOG("Schd Auction time : " << scheduled_auction_time);
  IEX_LOG("Book clear price  : " << auction_book_clearing_price);
  IEX_LOG("Collar ref price  : " << collar_reference_price);
  IEX_LOG("Lwr Auction collar: " << lower_auction_collar);
  IEX_LOG("Upr Auction collar: " << upper_auction_collar);
}

bool PriceLevelUpdateMessage::Decode(const uint8_t* data_ptr) {
  flags = GetNumeric<uint8_t>(data_ptr, 1);
  timestamp = GetNumeric<uint64_t>(data_ptr, 2);
  symbol = GetString(data_ptr, 10, 8);
  size = GetNumeric<uint32_t>(data_ptr, 18);
  price = GetPrice(data_ptr, 22);

  return ValidateTimestamp(timestamp);
}

void PriceLevelUpdateMessage::Print() const {
  IEX_LOG("Message type      : " << MessageTypeToString(message_type));
  IEX_LOG("Timestamp         : " << timestamp);
  IEX_LOG("Symbol            : " << symbol);
  IEX_LOG("Flag              : " << PRINTHEX(flags));
  IEX_LOG("Size              : " << size);
  IEX_LOG("Price             : " << price);
}

bool SecurityEventMessage::Decode(const uint8_t* data_ptr) {
  security_event =
      static_cast<SecurityEventMessage::SecurityMessageType>(GetNumeric<uint8_t>(data_ptr, 1));
  timestamp = GetNumeric<uint64_t>(data_ptr, 2);
  symbol = GetString(data_ptr, 10, 8);

  return ValidateTimestamp(timestamp);
}

void SecurityEventMessage::Print() const {
  IEX_LOG("Message type      : " << MessageTypeToString(message_type));
  IEX_LOG("Timestamp         : " << timestamp);
  IEX_LOG("Symbol            : " << symbol);
  IEX_LOG("SecurityEvent     : " << static_cast<char>(security_event));
}

std::unique_ptr<IEXMessageBase> IEXMessageFactory(const uint8_t* msg_data_ptr) {
  int msg_type = *msg_data_ptr;
  auto msg_enum = static_cast<MessageType>(msg_type);
  switch (msg_enum) {
    case MessageType::QuoteUpdate:
      return std::unique_ptr<IEXMessageBase>(new QuoteUpdateMessage());
    case MessageType::TradingStatus:
      return std::unique_ptr<IEXMessageBase>(new TradingStatusMessage());
    case MessageType::SystemEvent:
      return std::unique_ptr<IEXMessageBase>(new SystemEventMessage());
    case MessageType::SecurityDirectory:
      return std::unique_ptr<IEXMessageBase>(new SecurityDirectoryMessage());
    case MessageType::OperationalHaltStatus:
      return std::unique_ptr<IEXMessageBase>(new OperationalHaltStatusMessage());
    case MessageType::ShortSalePriceTestStatus:
      return std::unique_ptr<IEXMessageBase>(new ShortSalePriceTestStatusMessage());
    case MessageType::TradeReport:
    case MessageType::TradeBreak:
      return std::unique_ptr<IEXMessageBase>(new TradeReportMessage(msg_enum));
    case MessageType::OfficialPrice:
      return std::unique_ptr<IEXMessageBase>(new OfficialPriceMessage());
    case MessageType::AuctionInformation:
      return std::unique_ptr<IEXMessageBase>(new AuctionInformationMessage());
    case MessageType::PriceLevelUpdateBuy:
    case MessageType::PriceLevelUpdateSell:
      return std::unique_ptr<IEXMessageBase>(new PriceLevelUpdateMessage(msg_enum));
    case MessageType::SecurityEvent:
      return std::unique_ptr<IEXMessageBase>(new SecurityEventMessage(msg_enum));
    default:
      return NULL;
  }
  return NULL;
}
