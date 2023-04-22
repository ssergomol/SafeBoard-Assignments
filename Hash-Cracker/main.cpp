#include <iostream>
#include <vector>
#include <cstdio>
#include <fstream>
#include <thread>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include "thread_pool.cpp"

class HashCracker {
public:
    HashCracker(const std::string& md5Hash, const std::string& configFile) :
            md5Hash(md5Hash), configFile(configFile), pool(std::thread::hardware_concurrency()) {
        std::ifstream ifs(configFile);
        char c;
        while (ifs >> c) {
            charset.push_back(c);
        }
        ifs.close();
        std::sort(charset.begin(), charset.end());

    }

    HashCracker() : pool(std::thread::hardware_concurrency()) {
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
        std::remove("arguments.txt");
        std::remove("state.txt");

        currentLength = states[states.size() - 1];

        std::ifstream ifs(configFile);
        char c;
        while (ifs >> c) {
            charset.push_back(c);
        }
        ifs.close();
        std::sort(charset.begin(), charset.end());
    }

    void resume() {
        for (size_t i = 0; i < states.size() - 1; i++) {
            pool.add_task(&HashCracker::checkAllCombinations, this, states[i]);
        }
        states.clear();
        crack();
    }


    void crack() {
        std::thread readThread(&HashCracker::readWorker, this);

        for (; !matchFound && !stopRequested; currentLength++) {
            pool.add_task(&HashCracker::checkAllCombinations, this, currentLength);

            if (currentLength >= std::thread::hardware_concurrency()) {
                std::unique_lock<std::mutex> lock(finishedMutex);
                cv.wait(lock);
            }
        }

        if (stopRequested) {
            pool.wait_all();
            states.push_back(currentLength);
            std::ofstream outfile("state.txt", std::ios::out | std::ios::binary);
            outfile.write(reinterpret_cast<char *>(states.data()), states.size() * sizeof(int));
            outfile.close();
            std::ofstream argFile("arguments.txt", std::ios::out | std::ios::binary);
            argFile << md5Hash << " " << configFile;
            argFile.close();
        } else if (matchFound) {
            std::cout << matchingString << std::endl;
        }

        readThread.detach();
    }

private:

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

            // create current string
            for (int i = 0; i < length; i++) {
                originalString += charset[indices[i]];
            }

            if (md5(originalString) == md5Hash) {
                if (matchMutex.try_lock()) {
                    matchFound = true;
                    matchingString = originalString;
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
        finishedTasks.fetch_add(1);
        cv.notify_one();
    }

    void readWorker() {
        while (std::cin.get() != '\n') {
        }
        stopRequested = true;
    }

    // MD% hash function
    static std::string md5(const std::string &str) {
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

private:
    ThreadPool pool;
    std::vector<int> states;
    std::vector<char> charset;
    std::string md5Hash;
    std::string configFile;

    std::mutex matchMutex;
    std::mutex finishedMutex;
    std::mutex stateLock;
    std::atomic<int> finishedTasks{0};
    bool stopRequested = false;
    bool matchFound = false;
    std::string matchingString;
    std::condition_variable cv;
    size_t currentLength{1};
};

int main(int argc, char *argv[]) {
    if (argc == 2 && std::string(argv[1]) == "resume") {
        HashCracker cracker;
        cracker.resume();

    } else if (argc == 3) {
        HashCracker cracker(argv[1], argv[2]);
        cracker.crack();

    } else {
        std::cout << "Usage: inverse_md5_calc.exe <md5 hash> <config file>" << std::endl;
        return 1;
    }

    return 0;
}
