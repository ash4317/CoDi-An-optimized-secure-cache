#include <iostream>
#include <sstream>
// #include <openssl/sha.h>
#include <random>
#include "util.h"

using namespace std;

int line_to_set_mapping(uns64 addr, uns64 key, int num_sets) {
    
    // printf("Addr: %lu, Key: %lu\n", addr, key);
    // cout << addr << endl;
    // stringstream ss;
    // ss << addr << key;
    // string num_str = ss.str();

    // unsigned char hash[SHA256_DIGEST_LENGTH];
    // SHA256((const unsigned char*)num_str.c_str(), num_str.length(), hash);

    // // cout << "SHA256 hash of " << addr << " is: ";
    // // for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    // //     printf("%02x", hash[i]);
    // // }
    // // cout << endl;

    // uns64 hash_int = 0;
    // for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    //     hash_int = (hash_int << 8) | hash[i];
    // }
    // printf("Hash int: %lu\n", hash_int);

    // return int(hash_int % num_sets);

    // not using SHA256, using simple XOR
    return (addr ^ key) % num_sets;
}

int random_skew(int num_skews) {
    const int range_from  = 0;
    const int range_to = num_skews - 1;
    random_device rand_dev;
    mt19937 generator(rand_dev());
    uniform_int_distribution<int> distr(range_from, range_to);
    return distr(generator);
}