#include <iostream>
#include <sstream>
#include <fstream>

#include <curl/curl.h>
#include <json/json.h>

namespace
{
std::size_t cb(
    const char *in,
    std::size_t size,
    std::size_t num,
    char *out)
{
    std::string data(in, (std::size_t)size * num);
    *((std::stringstream *)out) << data;
    return size * num;
}
} // namespace

struct progress
{
    CURL *curl;
};

#define ONE_KILOBYTE CURL_OFF_T_C(1024)
#define ONE_MEGABYTE (CURL_OFF_T_C(1024) * 1024)

static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
    fprintf(stderr, "UP: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T " MB\r", ulnow / ONE_MEGABYTE, ultotal / ONE_MEGABYTE);
    return 0;
}

void showUsage(std::string name)
{
    std::cerr << "Teknik Upload Script in C++\n"
              << "Usage : " << name << " <option(s)>\n"
              << "Options:\n"
              << "\t-h, --help\t Display this message.\n"
              << "\t-d, --deletion\t Generate a deletion key.\n";
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

        struct progress prog;

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.teknik.io/v1/Upload");
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

        std::stringstream httpData;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &httpData);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
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
            Json::CharReaderBuilder jsonReader;
            std::string errs;
            if (Json::parseFromStream(jsonReader, httpData, &jsonData, &errs))
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

bool check_file_exists(const char *filename)
{
    std::ifstream infile(filename);
    return infile.good();
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
            std::string filename = argv[2];
            if (check_file_exists(filename.c_str()))
            {
                genDeletionKey = "true";
                fileUpload(genDeletionKey, argv[2]);
            }
            else
            {
                std::cout << "File doesn't exists!\n";
            }
        }
        else
        {
            std::cerr << "-d/--deletion option requires one argument." << std::endl;
            return 1;
        }
    }
    else
    {
        if (check_file_exists(arg.c_str()))
        {
            fileUpload(genDeletionKey, arg);
        }
        else
        {
            std::cout << "File doesn't exists!\n";
        }
    }
    return 0;
}
