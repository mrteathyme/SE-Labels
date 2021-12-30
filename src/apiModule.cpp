#include <curl/curl.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>
#include <json/json.h>


using namespace std;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


int main(int argc, char **argv)
{
    CURL* curl = curl_easy_init();
    if (!curl) {
        cerr << "curl initialization failure" << endl;
    }
    CURLcode res;
    /* static string readBuffer; */
    std::string readBuffer;

    string endPoint = argv[1];
    string channelID = argv[2];
    string jwtToken = argv[3];
    string url = "https://api.streamelements.com/kappa/v2/" + endPoint + "/" + channelID;
    string authKey = "Authorization: Bearer " + jwtToken;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, authKey.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    /*curl_easy_setopt(curl, CURLOPT_CAINFO, "ca-bundle.crt");*/
    cerr << "hi i shouldve used a debugger for this" << endl;
    res = curl_easy_perform(curl);
    cerr << readBuffer << endl;
    Json::CharReaderBuilder builder {};
    auto reader = std::unique_ptr<Json::CharReader>( builder.newCharReader() );
    Json::Value root {};
    std::string errors {};
    const auto is_parsed = reader->parse( readBuffer.c_str(),
                                          readBuffer.c_str() + readBuffer.length(),
                                          &root,
                                          &errors );
    if ( !is_parsed )
    {
        std::cerr << "ERROR: Could not parse! " << errors << '\n';
        return -1;
    }
    
    if (res != CURLE_OK) {
        cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
    }
    for( Json::Value::const_iterator outer = root.begin() ; outer != root.end() ; outer++ )
    {
        for( Json::Value::const_iterator inner = (*outer).begin() ; inner!= (*outer).end() ; inner++ )
        {
        cout << inner.key() << ": " << *inner << endl;
        }
    }

    curl_easy_cleanup(curl);
}