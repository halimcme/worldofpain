# Example code to send in-game event notifications to Discord.

## Requirements for this example code:
- C++ (only tested with C++17)
- JSON for Modern C++ library: https://github.com/nlohmann/json
- Machine Zone IXWebSocket library: https://github.com/machinezone/IXWebSocket (tested w/v5.1.3-11.4.3, build with cmake -DUSE_TLS=1 .)
- Create Discord webhooks for channels you want to send to, replace the channels and URLs in the example function with those: https://support.discord.com/hc/en-us/articles/228383668-Intro-to-Webhooks

### notify_discord function:

```
// send JSON RPC post to Discord bot
void notify_discord(string message, string channel)
{
  extern bool DEBUG_INSTANCE;
  if (DEBUG_INSTANCE) // disable if running under debugger
    return;

  ix::HttpClient httpClient;
  std::stringstream ss;

  string url;
  if (channel == "general")
    url = "https://discord.com/api/webhooks/yourDiscordGeneralWebhook";
  else if (channel == "logins")
    url = "https://discord.com/api/webhooks/yourDiscordLoginsWebhook";
  else if (channel == "immortals") {
    url = "https://discord.com/api/webhooks/yourDiscordImmortalsWebook";
  } else {
    cerr << "ERROR: notify_discord unknown channel: " << channel << endl;
    return;
  }

  auto args = httpClient.createRequest();
  args->extraHeaders["Content-Type"] = "application/json";

  nlohmann::json body;
  body["content"] = message;


  auto response = httpClient.post(url, body.dump(), args);

  auto statusCode = response->statusCode; // Can be HttpErrorCode::Ok, HttpErrorCode::UrlMalformed, etc...
  auto errorCode = response->errorCode;   // 200, 404, etc...
  auto responseHeaders =
      response->headers;              // All the headers in a special case-insensitive unordered_map of (string, string)
  auto responseBody = response->body; // All the bytes from the response as an std::string
  auto errorMsg = response->errorMsg; // Descriptive error message in case of failure
  auto uploadSize = response->uploadSize;     // Byte count of uploaded data
  auto downloadSize = response->downloadSize; // Byte count of downloaded data

  // uncomment for debugging responses
  //cerr << "notify_discord status:" << statusCode << " response: " << responseBody << " error: " << errorMsg << endl;
}
```
Recommended: call this function via a background thread so the http request doesn't pause gameplay.
