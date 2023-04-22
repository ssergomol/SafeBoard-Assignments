#include <iostream>
#include <vector>
#include <fstream>
#include <future>
#include <thread>
#include <condition_variable>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include "thread_pool.cpp"

std::vector<int> states;
std::vector<char> charset;
std::string md5Hash;
std::string configFile;

// Variables for parallel processing
std::mutex matchMutex;
std::mutex finishedMutex;
std::mutex stateLock;
std::atomic<int> finishedTasks{0};
bool stopRequested = false;
bool matchFound = false;
std::string matchingString;
std::condition_variable cv;
std::condition_variable readCV;
size_t currentLength{1};

// MD5 calculation function
std::string md5(const std::string &str) {
    unsigned char md[MD5_DIGEST_LENGTH];
    EVP_MD_CTX *mdCtx;
    const EVP_MD *mdType = EVP_md5();

    mdCtx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdCtx, mdType, NULL);
    EVP_DigestUpdate(mdCtx, str.c_str(), str.size());
    EVP_DigestFinal_ex(mdCtx, md, NULL);
    EVP_MD_CTX_free(mdCtx);

    char md5_str[2 * MD5_DIGEST_LENGTH + 1] = {0};
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&md5_str[i * 2], "%02x", (unsigned int) md[i]);
    }
    return {md5_str};
}

void checkAllCombinations(int length) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::string originalString;
    std::vector<int> indices(length, 0);
    while (true) {
        if (matchFound) {
            break;
        } else if (stopRequested) {
            std::lock_guard<std::mutex> lock(stateLock);
            states.push_back(length);
            break;
        }

        // print current string
        for (int i = 0; i < length; i++) {
            originalString += charset[indices[i]];
        }

        if (md5(originalString) == md5Hash) {
            if (matchMutex.try_lock()) {
                matchFound = true;
                matchingString = originalString;
                std::cout << "found\n";
                matchMutex.unlock();
            }
            break;
        }

        // increment indices
        int i = length - 1;
        while (i >= 0 && indices[i] == charset.size() - 1) {
            indices[i] = 0;
            i--;
        }
        if (i < 0) {
            break;
        }
        indices[i]++;
    }
    std::cout << "almost returned " << length << '\n';

    finishedTasks.fetch_add(1);
    cv.notify_one();
}

void readWorker() {
    while (std::cin.get() != '\n') {
    }
    stopRequested = true;
}

// Main function
int main(int argc, char *argv[]) {
    // Parse command-line arguments
    if (argc == 2 && std::string(argv[1]) == "resume") {
        std::ifstream infile("state.txt", std::ios::in | std::ios::binary);
        infile.seekg(0, std::ios::end);
        std::streampos file_size = infile.tellg();
        infile.seekg(0, std::ios::beg);

        states = std::vector<int>(file_size / sizeof(int));
        infile.read(reinterpret_cast<char *>(states.data()), file_size);
        infile.close();

        std::ifstream argumentFile("arguments.txt");
        argumentFile >> md5Hash >> configFile;
        argumentFile.close();

        for (auto item : states) {
            std::cout << item << " ";
        }
        std::cout << md5Hash << std::endl << configFile << std::endl;
        currentLength = states[states.size() - 1];

        std::ifstream ifs(configFile);
        char c;
        while (ifs >> c) {
            charset.push_back(c);
        }
        ifs.close();
        std::sort(charset.begin(), charset.end());


        ThreadPool pool(std::thread::hardware_concurrency());
        for (size_t i = 0; i < states.size() - 1; i++) {
            pool.add_task(checkAllCombinations, states[i]);
        }
        states.clear();
        std::thread readThread(readWorker);

        size_t counter = 0;
        for (; !matchFound && !stopRequested; currentLength++) {
            pool.add_task(checkAllCombinations, currentLength);
            std::cout << "add\n";
            if (currentLength >= std::thread::hardware_concurrency()) {
                std::unique_lock<std::mutex> lock(finishedMutex);
                std::cout << "Go to sleep\n";
                cv.wait(lock);
//                cv.wait(lock, [&](){return currentLength - finishedTasks.load()
//                <= std::thread::hardware_concurrency() + 1;});
//                counter = 0;
            }
        }
        std::cout << "wake up\n";

        if (stopRequested) {
            pool.wait_all();
            states.push_back(currentLength);
            std::ofstream outfile("state.txt", std::ios::out | std::ios::binary);
            outfile.write(reinterpret_cast<char *>(states.data()), states.size() * sizeof(int));
            outfile.close();
            std::ofstream argFile("arguments.txt", std::ios::out | std::ios::binary);

            // Write the strings to the file, separated by a space
            argFile << md5Hash << " " << configFile;
            argFile.close();

            readThread.join();

            return 0;
        } else if (matchFound) {
            std::cout << matchingString << std::endl;
        }

        std::cout << "Current length: " << currentLength << std::endl;
        readThread.detach();
        std::cout << "readthread joined\n";

    } else if (argc == 3) {
        md5Hash = std::string(argv[1]);
        configFile = std::string(argv[2]);
        std::ifstream ifs(configFile);
        char c;
        while (ifs >> c) {
            charset.push_back(c);
        }
        ifs.close();
        std::sort(charset.begin(), charset.end());

        ThreadPool pool(std::thread::hardware_concurrency());
        std::thread readThread(readWorker);

        size_t counter = 0;
        for (; !matchFound && !stopRequested; currentLength++) {
            pool.add_task(checkAllCombinations, currentLength);

            if (currentLength >= std::thread::hardware_concurrency()) {
                std::unique_lock<std::mutex> lock(finishedMutex);
                cv.wait(lock);
//                cv.wait(lock, [&](){return currentLength - finishedTasks.load()
//                <= std::thread::hardware_concurrency() + 1;});
//                counter = 0;
            }
        }

        if (stopRequested) {
            pool.wait_all();
            states.push_back(currentLength);
            std::ofstream outfile("state.txt", std::ios::out | std::ios::binary);
            outfile.write(reinterpret_cast<char *>(states.data()), states.size() * sizeof(int));
            outfile.close();
            std::ofstream argFile("arguments.txt", std::ios::out | std::ios::binary);

            // Write the strings to the file, separated by a space
            argFile << md5Hash << " " << configFile;
            argFile.close();

            readThread.join();
            return 0;
        } else if (matchFound) {
            std::cout << matchingString << std::endl;
        }

        std::cout << "Current length: " << currentLength << std::endl;
        readThread.detach();
        std::cout << "readthread joined\n";

    } else {
        std::cout << "Usage: inverse_md5_calc.exe <md5 hash> <config file>" << std::endl;
        return 1;
    }


    return 0;
}
