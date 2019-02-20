#include <iostream>
#include <bitset>
#include <vector>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <map>

class bitqueue{
public:
    bitqueue() {
        /*
         * byte = bits[7,6,5,4,3,2,1,0]
         * keep track of which bit is the beginning using iterator
         */
        bitvec = {7,6,5,4,3,2,1,0};
        bititerator = bitvec.begin();
        frontbit = *bititerator;

        /*
         * shifted data will span 2 bytes
         * mask of bitfields_first[front bit] + bitfields_second[front bit]
         */
        bitfields_first = {
                0b00000001,
                0b00000011,
                0b00000111,
                0b00001111,
                0b00011111,
                0b00111111,
                0b01111111,
                0b11111111,
        };
        bitfields_second = {
                0b11111110,
                0b11111100,
                0b11111000,
                0b11110000,
                0b11100000,
                0b11000000,
                0b10000000,
                0b00000000,
        };
    };

    void push(uint8_t byte){
        queue.push(byte);
    }

    bool empty(){
        return queue.empty();
    }

    /*
     * Circular list of 7,6,5,4,3,2,1
     * Used to index into the bitmasks, or for access in a bitset
     */
    int cycle_bit(){
        if(bititerator == bitvec.end()) {
            bititerator = bitvec.begin();
        }
        return *bititerator++;
    }

    /*
     * Return consecutive bits in a byte, consume the byte once all bits have been accessed
     */
    bool pop_bit(){
        uint8_t front = queue.front();
        std::bitset<8> bits(front);
        if(bititerator == bitvec.end()){
            bititerator = bitvec.begin();
            queue.pop();
        }
        frontbit = *bititerator++;
        return bits[frontbit];
    }

    /*
     * Return consecutive bytes given the shift by frontbit
     */
    uint8_t pop_byte(){
        uint8_t byte = 0;
        if(frontbit != 7){
            byte = (queue.front() & bitfields_first[frontbit]) << (7-frontbit);
            queue.pop();
            byte |= ((queue.front() & bitfields_second[frontbit])) >> (frontbit+1);
        }else{
            byte = queue.front();
            queue.pop();
        }
        return byte;
    }

    /*
     * Cycles through the bitmasks to find a specific byte given two bytes to work with
     * Consumes one byte from the queue
     * Sets the member frontbit which is used by find_next_byte
     */
    bool find_byte(uint8_t findbyte){
        if(queue.size() < 2){
            return false;
        }
        if(queue.front() == findbyte){
            queue.pop();
            return true;
        }else{
            uint8_t byte = 0;
            uint8_t byte1 = queue.front();
            queue.pop();
            uint8_t byte2 = queue.front();
            for(int i = 0; i < 8; i++){
                frontbit = cycle_bit();
                byte  = (byte1 & bitfields_first[frontbit]) << (7-frontbit);
                byte |= (byte2 & bitfields_second[frontbit]) >> (frontbit+1);
                if(byte == findbyte){
                    return true;
                }
            }
            frontbit = cycle_bit();
            return false;
        }
    }

    /*
     * Uses the previously found bit shift to construct another byte and compare it to a byte
     * Consumes one byte from the queue
     */
    bool find_next_byte(uint8_t findbyte){
        uint8_t byte = 0;
        if(frontbit != 7 && queue.size() >= 2){
            byte = (queue.front() & bitfields_first[frontbit]) << (7-frontbit);
            queue.pop();
            byte |= (queue.front() & bitfields_second[frontbit]) >> (frontbit+1);
        }else if (frontbit == 7){
            byte = queue.front();
            queue.pop();
        }
        return byte == findbyte;
    }

    /*
     * Given a vector of bytes, find the first one, then the remaining
     * Consumes all bytes of a byte aligned syncword, otherwise, the last byte remains
     */
    bool findsync(std::vector<uint8_t> bytes){
        std::vector<uint8_t>::iterator sync_byte = bytes.begin();
        while(!queue.empty()){
            while(!find_byte(*sync_byte)){
                //read new data
            }
            sync_byte++;
            while(find_next_byte(*sync_byte++)){
                if(sync_byte == bytes.end()){
                    return true;
                }
            }
            sync_byte = bytes.begin();
        }
        return false;
    }

private:
    std::queue<uint8_t> queue;
    std::vector<int> bitvec;
    std::vector<int>::iterator bititerator;
    int frontbit;
    std::vector<uint8_t> bitfields_second;
    std::vector<uint8_t> bitfields_first;

};


void parse_options(int argc, char **argv, std::map<std::string, std::string> &opts);

int main(int argc, char** argv) {
    std::map<std::string, std::string> options;
    parse_options(argc, argv, options);
    bitqueue q;
    q.push(0xA0);
    q.push(0xAA);
    q.push(0x55);
    q.push(0x01);
    q.push(0xA0);
    q.push(0xAA);
    q.push(0x55);
    q.push(0x01);
    q.push(0xA0);
    std::vector<uint8_t> syncword = {0xAA, 0x55};
    //std::cout << q.findsync(syncword) << std::endl;
    while(!q.empty()){
        if(q.findsync(syncword)){
            printf("%02X\n", q.pop_byte());
            printf("%02X\n", q.pop_byte());
        }
    }
    return 0;
}
void parse_options(int argc, char **argv, std::map<std::string, std::string> &opts) {

    int c;

    while (1)
    {
        static struct option long_options[] =
                {
                        {"help",     no_argument,       0, 'h'},
                        {"sync",  required_argument, 0, 's'},
                        {"framelength",  required_argument, 0, 'l'},
                        {"filename",    required_argument, 0, 'f'},
                        {0, 0, 0, 0}
                };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "h?l:s:f:",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;

            case 's':
                opts["syncword"] = optarg;
                break;

            case 'l':
                opts["framelength"] = optarg;
                break;

            case 'f':
                opts["filename"] = optarg;
                break;

            case '?':
            case 'h':
                printf("--syncword -s \n--framelength -l \n--filename -f\n");
                /* getopt_long already printed an error message. */
                break;

            default:
                printf("default");
                abort ();
        }
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        putchar ('\n');
    }

}
