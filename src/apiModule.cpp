#include <curl/curl.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>
#include <json/json.h>
#include <fstream>
#include <boost/dll.hpp>
#include <boost/format.hpp>

#ifdef _WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif

using namespace std;
using boost::format;
/*using boost::io::group;*/

bool FindVariableString(
    const std::string &str,
    const std::string::size_type pos,
    std::string::size_type &beginVarStringPos,
    std::string::size_type &endVarStringPos,
    std::string::size_type &beginVarNamePos,
    std::string::size_type &endVarNamePos)
{
    const char *TestString = "%$#";
    const char PercentSign = '%';
    const char LeftParenthesis = '(';
    const char LeftSquareBracket = '[';
    const char LeftCurlyBracket = '{';
    const char RightParenthesis = ')';
    const char RightSquareBracket = ']';
    const char RightCurlyBracket = '}';
    beginVarStringPos = std::string::npos;
    endVarStringPos = std::string::npos;
    beginVarNamePos = std::string::npos;
    endVarNamePos = std::string::npos;
    if (str.empty())
    {
        return false;
    }
    beginVarStringPos = str.find_first_of(TestString, pos);
    if (std::string::npos == beginVarStringPos)
    {
        return false;
    }
    if (beginVarStringPos >= str.length() - 1)
    {
        return false;
    }
    char ch = str[beginVarStringPos];
    char ch1 = str[beginVarStringPos + 1];
    if (
       PercentSign == ch
       && LeftParenthesis != ch1 && LeftSquareBracket != ch1
       && LeftCurlyBracket != ch1
       )
    {
        beginVarNamePos = beginVarStringPos + 1;
        endVarStringPos = str.find(PercentSign, beginVarNamePos);
        if (std::string::npos == endVarStringPos)
        {
            return false;
        }
    }
    else if (
       LeftParenthesis != ch1 && LeftSquareBracket != ch1
       && LeftCurlyBracket != ch1
       )
    {
        return false;
    }
    else
    {
        beginVarNamePos = beginVarStringPos + 2;
        char closeChar = 0;
        if (LeftParenthesis == ch1)
        {
            closeChar = RightParenthesis;
        }
        else if (LeftSquareBracket == ch1)
        {
            closeChar = RightSquareBracket;
        }
        else if (LeftCurlyBracket == ch1)
        {
            closeChar = RightCurlyBracket;
        }
        endVarStringPos = str.find(closeChar, beginVarNamePos);
        if (std::string::npos == endVarStringPos)
        {
            return false;
        }
    }
    endVarNamePos = endVarStringPos - 1;
    return true;
}

bool StringContainsVariableStrings(const std::string &str)
{
    std::string::size_type beginVarStringPos = 0;
    std::string::size_type endVarStringPos = 0;
    std::string::size_type beginVarNamePos = 0;
    std::string::size_type endVarNamePos = 0;
    bool ret = FindVariableString(str, 0, beginVarStringPos, endVarStringPos, beginVarNamePos, endVarNamePos);
    return ret;
}
 
std::string GetVariableValue(
    const std::string &varName,
    const std::map<std::string, std::string> &env,
    bool &fromEnvMap, bool &valueContainsVariableStrings)
{
    typedef std::map<std::string, std::string> my_map;
    fromEnvMap = false;
    valueContainsVariableStrings = false;
    std::string ret;
    my_map::const_iterator itFind = env.find(varName);
    if (itFind != env.end())
    {
        ret = (*itFind).second;
        if (!ret.empty())
        {
            fromEnvMap = true;
            valueContainsVariableStrings = StringContainsVariableStrings(ret);
        }
    }
    
    if (ret.empty())
    {
        void * env = ::getenv(varName.c_str());
        if (env == NULL){
            return "";
        }
        ret = *static_cast<std::string*>(env);
    }
    return ret;
}

std::string ExpandVars(
    const std::string &original,
    const std::map<std::string, std::string> &env)
{
    std::string ret = original;
    if (original.empty())
    {
        return ret;
    }
    bool foundVar = false;
    std::string::size_type pos = 0;
    do
    {
        std::string::size_type beginVarStringPos = 0;
        std::string::size_type endVarStringPos = 0;
        std::string::size_type beginVarNamePos = 0;
        std::string::size_type endVarNamePos = 0;
        foundVar = FindVariableString(ret, pos, beginVarStringPos, endVarStringPos, beginVarNamePos, endVarNamePos);
        if (foundVar)
        {
            std::string::size_type varStringLen = endVarStringPos - beginVarStringPos + 1;
            std::string varString = ret.substr(beginVarStringPos, varStringLen);
            std::string::size_type varNameLen = endVarNamePos - beginVarNamePos + 1;
            std::string varName = ret.substr(beginVarNamePos, varNameLen);
            bool fromEnvMap;
            bool valueContainsVariableStrings;
            std::string value = GetVariableValue(varName, env, fromEnvMap, valueContainsVariableStrings);
            if (!value.empty())
            {
                ret = ret.replace(beginVarStringPos, varStringLen, value);
                pos = beginVarStringPos;
            }
            else
            {
                pos = endVarStringPos + 1;
            }
        }
    } while (foundVar);
    return ret;
}





static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int extractJsonToFile(Json::Value json, string fileName, string element, std::string format)
{   
    map<string, string> data;
    Json::Value subElements = json.get("data", "You broke something you fool" ).get(element, "You broke something again foo");

    for(Json::Value::const_iterator itr = subElements.begin(); itr !=  subElements.end(); itr++) {
        data[itr.key().asString()] = json.get("data", "You broke something you fool" ).get(element, "You broke something again foo").get(itr.key().asString(), "You broke something again foo").asString();
    }
   std::string dataToWrite;
   try{
    dataToWrite = ExpandVars(format, data);
   }
   catch(...) {
    dataToWrite = "N/A";
   }
    ofstream file(fileName);
    file << dataToWrite;
    file.close();
    return 0;
}

map<string, string> extractJsonData(Json::Value json, string element)
{   
    map<string, string> data;
    Json::Value subElements = json.get("data", "You broke something you fool" ).get(element, "You broke something again foo");

    for(Json::Value::const_iterator itr = subElements.begin(); itr !=  subElements.end(); itr++) {
        data[itr.key().asString()] = json.get("data", "You broke something you fool" ).get(element, "You broke something again foo").get(itr.key().asString(), "You broke something again foo").asString();
    }

    return data;
}


int main(int argc, char **argv)
{
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        cerr << "curl initialization failure" << endl;
    }
    map<string, string> followerDataOld;
    map<string, string> subDataOld;
    map<string, string> tipDataOld;
    
    for(;;){
    CURLcode res;
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
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    res = curl_easy_perform(curl);
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
    if (readBuffer != "[]") {

    extractJsonToFile(root, "follower-latest.txt", "follower-latest", "Latest Follower: ${name}");
    extractJsonToFile(root, "subscriber-latest.txt", "subscriber-latest", "Latest Subscriber: ${name}");
    extractJsonToFile(root, "tip-latest.txt", "tip-latest", "Latest Tipper: ${name} - Amount: ${amount}");
    map<string, string> followerDataNew = extractJsonData(root, "follower-latest");
    map<string, string> subDataNew = extractJsonData(root, "subscriber-latest");
    map<string, string> tipDataNew = extractJsonData(root, "tip-latest");
    bool newFollower = followerDataNew != followerDataOld;
    bool newSub = subDataNew != subDataOld;
    bool newTip = tipDataNew != tipDataOld;
    followerDataOld = followerDataNew;
    subDataOld = subDataNew;
    tipDataOld = tipDataNew;
    if(newTip){
        cerr << "New Tip: Name = " << tipDataOld["name"] << " Amount = $" << tipDataOld["amount"] << endl;
    }
    if(newSub){
        cerr << "New Sub: Name = " << subDataOld["name"] << endl;
    }
    if(newFollower){
        cerr << "New Follower: Name = " << followerDataOld["name"] << endl;
    }
    Sleep(1000);
    }
    }
}