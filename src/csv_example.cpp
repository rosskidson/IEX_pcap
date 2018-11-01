#include "iex_decoder.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
  // Get the input pcap file as an argument.
  if (argc < 2) {
    std::cout << "Usage: iex_pcap_decoder <input_pcap>" << std::endl;
    return 1;
  }

  // Open a file stream for writing output to csv.
  std::ofstream out_stream;
  try {
    out_stream.open("quotes.csv");
  } catch (...) {
    std::cout << "Exception thrown opening output file." << std::endl;
    return 1;
  }

  // Add the header.
  out_stream << "Timestamp,Symbol,BidSize,BidPrice,AskSize,AskPrice" << std::endl;

  // Initialize decoder object with file path.
  std::string input_file(argv[1]);
  IEXDecoder decoder;
  if (!decoder.OpenFileForDecoding(input_file)) {
    std::cout << "Failed to open file '" << input_file << "'." << std::endl;
    return 1;
  }

  // Get the first message from the pcap file.
  std::unique_ptr<IEXMessageBase> msg_ptr;
  auto ret_code = decoder.GetNextMessage(msg_ptr);

  // Main loop to loop through all messages.
  for (; ret_code == ReturnCode::Success; ret_code = decoder.GetNextMessage(msg_ptr)) {

    // For quick message introspection:
    // msg_ptr->Print();
    // Uncommenting this will completely dominate your terminal with output.

    // There are many different message types. Here we just look for quote update (L1 tick).
    if (msg_ptr->GetMessageType() == MessageType::QuoteUpdate) {

      // Cast it to the derived type.
      auto quote_msg = dynamic_cast<QuoteUpdateMessage*>(msg_ptr.get());

      // Check the pointer and write all L1 ticks for ticker 'AMD' to file.
      if (quote_msg && quote_msg->symbol == "AMD") {
        out_stream << quote_msg->timestamp << ","
                   << quote_msg->symbol << ","
                   << quote_msg->bid_size << ","
                   << quote_msg->bid_price << ","
                   << quote_msg->ask_size << ","
                   << quote_msg->ask_price << std::endl;
      }
    }
  }
  out_stream.close();
  return 0;
}
