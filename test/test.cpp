#include <iostream>
#include "gtest/gtest.h"

#include "iex_decoder.h"
#include "iex_messages.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

const std::string tops_pcap_filename = "20180127_IEXTP1_TOPS1.6.pcap";
const std::string deep_pcap_filename = "20180127_IEXTP1_DEEP1.0.pcap";
std::string tops_pcap_filepath = "";
std::string deep_pcap_filepath = "";

// The decoder class is used to store a decoder, as well as decoded data.
class DecoderTest : public ::testing::Test {
 protected:
  // void SetUp() override {}
  // void TearDown() override {}

  static std::vector<std::unique_ptr<IEXMessageBase>> msgs_;
  static IEXDecoder decoder_;
};

IEXDecoder DecoderTest::decoder_;

std::vector<std::unique_ptr<IEXMessageBase>> DecoderTest::msgs_;

inline bool file_exists(const std::string& name) {
  std::ifstream f(name.c_str());
  return f.good();
}

TEST(DataPresent, UnitTestPreTest) {
  // Search for the pcap data files using a few common paths. Throw an error if they are not found.
  // Note: This is a super hacky way of dealing with windows. I did it this way because I didn't
  // want to pull boost in for a single fs.join() and std::filesystem is too new for my compiler.
  std::vector<std::string> common_paths = {"", "data/", "../data/", "data\\", "..\\data\\"};
  bool success = false;
  for (const auto& test_path : common_paths) {
    tops_pcap_filepath = test_path + tops_pcap_filename;
    deep_pcap_filepath = test_path + deep_pcap_filename;
    std::cout << tops_pcap_filepath << std::endl;
    std::cout << deep_pcap_filepath << std::endl;
    if (file_exists(tops_pcap_filepath) && file_exists(deep_pcap_filepath)) {
      success = true;
      break;
    }
  }
  if (!success) {
    std::cout << "############################################################" << std::endl
              << "The data files " << tops_pcap_filename << std::endl
              << "               " << deep_pcap_filename << std::endl
              << " were not found. These are required for unit testing." << std::endl
              << "Please run the unit tests from the project root directory." << std::endl
              << "############################################################" << std::endl;
  }
  ASSERT_TRUE(success);
}

// Test initialization of the decoder.
TEST_F(DecoderTest, DecoderSetup) {
  // Test it fails without running open file for decoding first.
  std::unique_ptr<IEXMessageBase> msg_ptr;
  EXPECT_EQ(decoder_.GetNextMessage(msg_ptr), ReturnCode::ClassNotInitialized);

  // Test bad filename.
  EXPECT_FALSE(decoder_.OpenFileForDecoding("bad_filename.notafile"));

  // Test that opening succeeds for a normal file.
  ASSERT_TRUE(decoder_.OpenFileForDecoding(tops_pcap_filepath));
}

// Decode all messages and store all the messages in memory. This is for two reasons:
// 1) Allocated large amounts of memory will more likely expose any memory corruption issues.
// 2) Select messages can be sampled from the decoded data to be independently tested.
TEST_F(DecoderTest, DecodeTOPSTest) {
  constexpr int num_messages = 99871;
  for (int i = 0; i < num_messages; ++i) {
    std::unique_ptr<IEXMessageBase> msg_ptr;
    auto res = decoder_.GetNextMessage(msg_ptr);
    if (res != ReturnCode::Success) {
      std::cout << "Decoding error: " << ReturnCodeToString(res) << std::endl;
    }
    ASSERT_EQ(res, ReturnCode::Success);
    msgs_.emplace_back(std::move(msg_ptr));
  }
  // This tests that the stream does in fact end where it is expected to end.
  {
    std::unique_ptr<IEXMessageBase> msg_ptr;
    ASSERT_EQ(decoder_.GetNextMessage(msg_ptr), ReturnCode::EndOfStream);
  }
}

// Sample various messages and various message types and test the decoded data.

TEST_F(DecoderTest, HeaderTest) {
  // Test the first header.
  const IEXTPHeader& header = decoder_.GetFirstHeader();
  EXPECT_EQ(header.version, 1);
  EXPECT_EQ(header.protocol_id, 32771);
  EXPECT_EQ(header.channel_id, 1);
  EXPECT_EQ(header.session_id, 1150681088);
  EXPECT_EQ(header.payload_len, 0);
  EXPECT_EQ(header.message_count, 0);
  EXPECT_EQ(header.stream_offset, 0);
  EXPECT_EQ(header.first_msg_sq_num, 1);
  EXPECT_EQ(header.send_time, 1517058015909382289);

  // Test the last header.
  const IEXTPHeader& last_header = decoder_.GetLastDecodedHeader();
  EXPECT_EQ(last_header.version, 1);
  EXPECT_EQ(last_header.protocol_id, 32771);
  EXPECT_EQ(last_header.channel_id, 1);
  EXPECT_EQ(last_header.session_id, 1150681088);
  EXPECT_EQ(last_header.payload_len, 0);
  EXPECT_EQ(last_header.message_count, 0);
  EXPECT_EQ(last_header.stream_offset, 3870321);
  EXPECT_EQ(last_header.first_msg_sq_num, 99872);
  EXPECT_EQ(last_header.send_time, 1517074717381264091);
}

TEST_F(DecoderTest, SystemEventTest) {
  constexpr int msg_idx = 34268;
  ASSERT_GE(msgs_.size(), msg_idx);
  auto& msg_ptr = msgs_[msg_idx];
  auto system_msg = dynamic_cast<SystemEventMessage*>(msg_ptr.get());
  ASSERT_NE(system_msg, nullptr);
  EXPECT_EQ(system_msg->timestamp, 1517058017224122394);
  EXPECT_EQ(system_msg->system_event, SystemEventMessage::Code::StartOfSystemHours);
}

TEST_F(DecoderTest, SecurityDirectory) {
  constexpr int msg_idx = 34109;
  ASSERT_GE(msgs_.size(), msg_idx);
  auto& msg_ptr = msgs_[msg_idx];
  auto security_msg = dynamic_cast<SecurityDirectoryMessage*>(msg_ptr.get());
  ASSERT_NE(security_msg, nullptr);
  EXPECT_EQ(security_msg->timestamp, 1517058016638245341);
  EXPECT_EQ(security_msg->symbol, "ZEXIT");
  EXPECT_EQ(security_msg->flags, 128);
  EXPECT_EQ(security_msg->round_lot_size, 100);
  EXPECT_EQ(security_msg->adjusted_POC_price, 10);
  EXPECT_EQ(security_msg->LULD_tier, SecurityDirectoryMessage::LULDTier::Tier1NMSStock);
}

TEST_F(DecoderTest, QuoteMessageTest) {
  constexpr int msg_idx = 47270;
  ASSERT_GE(msgs_.size(), msg_idx);
  auto& msg_ptr = msgs_[msg_idx];
  auto quote_msg = dynamic_cast<QuoteUpdateMessage*>(msg_ptr.get());
  ASSERT_NE(quote_msg, nullptr);
  EXPECT_EQ(quote_msg->timestamp, 1517065649985331707);
  EXPECT_EQ(quote_msg->symbol, "AUO");
  EXPECT_EQ(quote_msg->flags, 0);
  EXPECT_EQ(quote_msg->bid_size, 1280);
  EXPECT_EQ(quote_msg->bid_price, 4.06);
  EXPECT_EQ(quote_msg->ask_size, 19232);
  EXPECT_EQ(quote_msg->ask_price, 4.34);
}

TEST_F(DecoderTest, TradeReportTest) {
  constexpr int msg_idx = 34344;
  ASSERT_GE(msgs_.size(), msg_idx);
  auto& msg_ptr = msgs_[msg_idx];
  auto report_msg = dynamic_cast<TradeReportMessage*>(msg_ptr.get());
  ASSERT_NE(report_msg, nullptr);
  EXPECT_EQ(report_msg->timestamp, 1517059857193914072);
  EXPECT_EQ(report_msg->symbol, "ZXIET");
  EXPECT_EQ(report_msg->flags, 192);
  EXPECT_EQ(report_msg->size, 100);
  EXPECT_EQ(report_msg->price, 99.97);
  EXPECT_EQ(report_msg->trade_id, 967187);
}

TEST_F(DecoderTest, OfficialPriceTest) {
  constexpr int msg_idx = 35581;
  ASSERT_GE(msgs_.size(), msg_idx);
  auto& msg_ptr = msgs_[msg_idx];
  auto price_msg = dynamic_cast<OfficialPriceMessage*>(msg_ptr.get());
  ASSERT_NE(price_msg, nullptr);
  EXPECT_EQ(price_msg->timestamp, 1517063400002535006);
  EXPECT_EQ(price_msg->symbol, "ZEXIT");
  EXPECT_EQ(price_msg->price_type, OfficialPriceMessage::PriceType::OpeningPrice);
  EXPECT_EQ(price_msg->price, 9.99);
}

TEST_F(DecoderTest, AuctionInformationTest) {
  constexpr int msg_idx = 35339;
  ASSERT_GE(msgs_.size(), msg_idx);
  auto& msg_ptr = msgs_[msg_idx];
  auto auction_msg = dynamic_cast<AuctionInformationMessage*>(msg_ptr.get());
  ASSERT_NE(auction_msg, nullptr);
  EXPECT_EQ(auction_msg->timestamp, 1517063280011278686);
  EXPECT_EQ(auction_msg->symbol, "ZEXIT");
  EXPECT_EQ(auction_msg->auction_type, AuctionInformationMessage::AuctionType::OpeningAuction);
  EXPECT_EQ(auction_msg->paired_shares, 907);
  EXPECT_EQ(auction_msg->reference_price, 10);
  EXPECT_EQ(auction_msg->indicative_clearing_price, 9.99);
  EXPECT_EQ(auction_msg->imbalance_shares, 2345);
  EXPECT_EQ(auction_msg->imbalance_side,
            AuctionInformationMessage::ImbalanceSide::SellSideImbalance);
  EXPECT_EQ(auction_msg->extension_number, 0);
  EXPECT_EQ(auction_msg->scheduled_auction_time, 1517063400);
  EXPECT_EQ(auction_msg->auction_book_clearing_price, 9.99);
  EXPECT_EQ(auction_msg->collar_reference_price, 10);
  EXPECT_EQ(auction_msg->lower_auction_collar, 9);
  EXPECT_EQ(auction_msg->upper_auction_collar, 11);
}

// Open up a second pcap file, to test DEEP decoding and specific message types.
TEST_F(DecoderTest, DecodeDEEPTest) {
  ASSERT_TRUE(decoder_.OpenFileForDecoding(deep_pcap_filepath));
  constexpr int num_messages = 105068;
  msgs_.clear();
  int j = 0;
  for (int i = 0; i < num_messages; ++i) {
    std::unique_ptr<IEXMessageBase> msg_ptr;
    auto res = decoder_.GetNextMessage(msg_ptr);
    if (res != ReturnCode::Success) {
      std::cout << "Decoding error: " << ReturnCodeToString(res) << std::endl;
    }
    ASSERT_EQ(res, ReturnCode::Success);
    msgs_.emplace_back(std::move(msg_ptr));
  }
  {
    std::unique_ptr<IEXMessageBase> msg_ptr;
    ASSERT_EQ(decoder_.GetNextMessage(msg_ptr), ReturnCode::EndOfStream);
  }
}

TEST_F(DecoderTest, PriceLevelUpdateTest) {
  constexpr int msg_idx = 25781;
  ASSERT_GE(msgs_.size(), msg_idx);
  auto& msg_ptr = msgs_[msg_idx];
  auto price_lvl_msg = dynamic_cast<PriceLevelUpdateMessage*>(msg_ptr.get());
  ASSERT_NE(price_lvl_msg, nullptr);
  EXPECT_EQ(price_lvl_msg->timestamp, 1517059883978005676);
  EXPECT_EQ(price_lvl_msg->symbol, "ZIEXT");
  EXPECT_EQ(price_lvl_msg->flags, 1);
  EXPECT_EQ(price_lvl_msg->size, 351);
  EXPECT_EQ(price_lvl_msg->price, 1);
}

TEST_F(DecoderTest, SecurityEventTest) {
  constexpr int msg_idx = 27017;
  ASSERT_GE(msgs_.size(), msg_idx);
  auto& msg_ptr = msgs_[msg_idx];
  auto security_msg = dynamic_cast<SecurityEventMessage*>(msg_ptr.get());
  ASSERT_NE(security_msg, nullptr);
  EXPECT_EQ(security_msg->timestamp, 1517063400001073818);
  EXPECT_EQ(security_msg->symbol, "AAPL");
  EXPECT_EQ(security_msg->security_event,
            SecurityEventMessage::SecurityMessageType::OpeningProcessComplete);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
