# IEX_pcap
C++ library for decoding stock market data from pcap files available from the IEX exchange.

### Brief explanation

The IEX stock market exchange (https://iextrading.com/) provides all historical data from their exchange, free to download, since end of 2016. The data is stored in .pcap files, so in order to access the data, the packets need to extracted from the pcap files and decoded into usable data using the spec released by IEX.  This library provides an interface to do exactly that.  There is no functionality to access IEX's web API, however there are already tonnes of libraries in all languages that do that listed on the website.

### Building

```
mkdir build
cd build
cmake .. && make
```

### Usage

Following is a minimal example to extract all the L1 ticks for the ticker AMD and output to csv.  This is included in the source.

``` c++
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
```

This library provides structs for all message types contained within the pcap files, both TOPS and DEEP.  For more information, read the documentation on the website https://iextrading.com/trading/market-data/ and have a look at include/iex_messages.h

### Dependencies

This project depends on gtest and pcapplusplus.  They are both pulled in using CMake's ExternalProject_Add so there shouldn't be anything to do, just have internet when you are building it.

### Compatibility

I have tested this on Mac and Linux.  There is no reason why it can't work on Windows, but I haven't tested it and I don't know anything about Windows build environments so it probably will need some tweaking.

### Testing

This project has unit tests.  Run test_iex from the build directory.

### Feature TODO list
- JSON serialization of all message types (https://stackoverflow.com/a/19974486)
- Automatic download of pcap files given a certain date
- Python bindings

I am not currently working on this project right now. If any of these features look interesting to you, please contact me.
