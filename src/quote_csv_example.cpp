#include "iex_decoder.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

std::string OutputCSVHeader() { return "Timestamp,Symbol,BidSize,BidPrice,AskSize,AskPrice"; }

std::string OutputToCSVLine(const QuoteUpdateMessage& msg) {
  std::stringstream ss;
  ss << msg.timestamp << "," << msg.symbol << "," << msg.bid_size << "," << msg.bid_price << ","
     << msg.ask_size << "," << msg.ask_price;
  return ss.str();
}

template <typename Derived, typename Base>
std::unique_ptr<Derived> dynamic_unique_ptr_cast(std::unique_ptr<Base>&& p) {
  if (Derived* result = dynamic_cast<Derived*>(p.get())) {
    p.release();
    return std::unique_ptr<Derived>(result);
  }
  return std::unique_ptr<Derived>(nullptr);
}

int main(int argc, char* argv[]) {
  // Get the input pcap file as an argument.
  if (argc < 2) {
    std::cout << "Usage: iex_pcap_decoder <input_pcap>" << std::endl;
    return 1;
  }

  std::string input_file(argv[1]);
  IEXDecoder decoder;
  if (!decoder.OpenFileForDecoding(input_file)) {
    std::cout << "Failed to open file '" << input_file << "'." << std::endl;
    return 1;
  }

  // For output.
  std::ofstream out_stream;
  try {
    out_stream.open("quotes.csv");
  } catch (...) {
    std::cout << "Exception thrown opening output file." << std::endl;
    return 1;
  }
  out_stream << OutputCSVHeader() << std::endl;

  std::unique_ptr<IEXMessageBase> msg_ptr;
  auto ret_code = decoder.GetNextMessage(msg_ptr);
  int msg_num = 0;
  for (; ret_code == ReturnCode::Success; ret_code = decoder.GetNextMessage(msg_ptr)) {
    // msg_ptr->Print();
    // std::cout << "-----------------------------" << std::endl;

    if (msg_ptr->GetMessageType() == MessageType::QuoteUpdate) {
      // Cast it to the derived type.
      std::unique_ptr<QuoteUpdateMessage> quote_msg =
          dynamic_unique_ptr_cast<QuoteUpdateMessage>(std::move(msg_ptr));
      // If the dynamic cast failed, that pointer is toast. Continue to the next message.
      if (!quote_msg) {
        continue;
      }

      if (quote_msg->symbol == "AMD") {
        out_stream << OutputToCSVLine(*quote_msg) << std::endl;
        if (!(msg_num++ % 1000)) {
          std::cout << "Processed " << msg_num << " messages" << std::endl;
        }
      }
    }
  }

  out_stream.close();

  return 0;
}
