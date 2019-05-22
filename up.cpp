#include <iostream>
#include <fstream>

#include <curl/curl.h>
#include <json/json.h>

namespace
{
size_t cb(
    const char *in,
    size_t size,
    size_t num,
    std::string *out)
{
    const size_t totalBytes(size * num);
    out->append(in, totalBytes);
    return totalBytes;
};
} // namespace

void showUsage(std::string name)
{
    std::cerr << "Teknik Upload Script in C++\n"
              << "Usage : " << name << " <option(s)>\n"
              << "Options:\n"
              << "\t-h, --help\t Display this message.\n"
              << "\t-d, --deletion\t Generate a deletion key.";
}

void fileUpload(std::string genDeletionKey, std::string filename)
{
    std::string contents;
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
    if (in)
    {
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
    }
    CURL *curl;
    CURLcode res;
    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastptr = NULL;
    curl_global_init(CURL_GLOBAL_ALL);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "genDeletionKey",
                 CURLFORM_COPYCONTENTS, genDeletionKey.c_str(), CURLFORM_END);
    curl_formadd(&formpost, &lastptr,
                 CURLFORM_COPYNAME, "file",
                 CURLFORM_BUFFER, "data",
                 CURLFORM_BUFFERPTR, contents.data(),
                 CURLFORM_BUFFERLENGTH, contents.size(),
                 CURLFORM_END);
    curl = curl_easy_init();
    if (curl)
    {
        int httpCode = 0;

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.teknik.io/v1/Upload");
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

        std::string *httpData(new std::string());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData);
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_formfree(formpost);
        if (httpCode == 200)
        {
            Json::Value jsonData;
            Json::Reader jsonReader;
            if (jsonReader.parse(*httpData, jsonData))
            {
                const std::string url(jsonData["result"]["url"].asString());
                const std::string deletionKey(jsonData["result"]["deletionKey"].asString());
                std::cout << "URL: " << url << std::endl;
                if (genDeletionKey == "true")
                {
                    std::cout << "Deletion key: " << deletionKey << std::endl;
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    std::string genDeletionKey = "false";
    if (argc < 2)
    {
        showUsage(argv[0]);
        return 1;
    }
    std::string arg = argv[1];
    if (arg == "-h" || arg == "--help")
    {
        showUsage(argv[0]);
        return 0;
    }
    else if (arg == "-d" || arg == "--deletion")
    {
        if (argv[2])
        {
            genDeletionKey = "true";
            fileUpload(genDeletionKey, argv[2]);
        }
        else
        {
            std::cerr << "-d/--deletion option requires one argument." << std::endl;
            return 1;
        }
    }
    else
    {
        fileUpload(genDeletionKey, arg);
    }
    return 0;
}