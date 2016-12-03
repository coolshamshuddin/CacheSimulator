/*
Cache Simulator
Level one L1 and level two L2 cache parameters are read from file (block size, line per set and set per cache).
The 32 bit address is divided into tag bits (t), set index bits (s) and block offset bits (b)
s = log2(#sets)   b = log2(block size)  t=32-s-b
*/
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <stdlib.h>
#include <cmath>
#include <bitset>

using namespace std;
//access state:
#define NA 0 // no action
#define RH 1 // read hit
#define RM 2 // read miss
#define WH 3 // Write hit
#define WM 4 // write miss



struct config{
       int L1blocksize;
       int L1setsize;
       int L1size;
       int L2blocksize;
       int L2setsize;
       int L2size;
       };

//Structure to model the blocks

struct block{
    int blockSize;
    bitset<32> startAddress;
    bool isValid;
};

//Structure for hit

struct hit{
    bool isHit;
    int hitWay;
};

//Structure to bundle the eviction data. Contains information if the write leads to an eviction or not.
//and if it does, returns the evicted block.

struct writeReturn{
    bool isEvicted;
    block evictedBlock;
};

int btoi(string);

/* you can define the cache class here, or design your own data structure for L1 and L2 cache
class cache {

      }
*/
class cache{

    //Given Attributes of a cache
    int cacheSize;
    int setSize;
    int blockSize;


    //To be computed attributes from the given data
    int sets;
    int ways;

    int tag;
    int index;
    int offset;

    //Round-robin counter for each set to store the information about the previous eviction.

    vector<int> counter;

    //List of blocks to store the information of the blocks such as the validity and the address of the data
    //the block contains.
    vector<block> blocks;

    //constructor
    public:
    cache(int cache_Size, int set_Size, int block_Size){
        //Initializing values
        cacheSize = cache_Size*1024;   //The provided cache size is in KiloBytes. Got to convert it into bytes.
        setSize = set_Size;            //Associativity or the number of ways
        blockSize = block_Size;

        //calculating number of sets and ways
        if(setSize == 0){       //If the setSize is 0, it is fully associative. Hence there is only one set
            sets = 1;
            ways = (cacheSize)/blockSize;
        }
        else{                   //Else the set size indicates the associativity i.e., the number of ways
            ways = setSize;
            sets = (cacheSize/ways)/blockSize;
        }
        //Number of blocks = Number of ways * Number of sets
        blocks.resize(ways*sets);
        //One round-robin counter per set
        counter.resize(sets);
        //Number of bits required for offset, index and tag
        offset = log2(blockSize);
        index = log2(sets);
        tag = 32 - offset - index;
        //Initialize all the round-robin counters to 0;
        cout<<offset<<" "<<index<<" "<<tag<<" ";

        for(int i=0; i<sets; i++){
            counter[i] = 0;
        }

        //Invalidate all the cache blocks. Done here in a set first basis
        for(int i=0; i<sets; i++){
            for(int j=0; j<ways; j++){
                blocks[(i*ways)+j].isValid = false;
            }
        }

        //Done with constructor. So all the parameters of the cache have been set.
    }


    //Operations a cache can perform.

    //A cache can be read or written.
    //Here are the helper methods which help you do so.
    //These helper methods are basically dumb and they read or write without actually checking any condition
    //and do not make any changes to the cache.
    //These conditions should be handled in the main method when these methods are accessed.


    //The Read method checks if the passed address is present in the cache
    //and returns an object of type 'hit' which contains the info if it is a hit or not.
    //And also the way in which it was a hit if it was a hit.

    hit Read(bitset<32> address){

        string addressString = address.to_string();
        string tagString = addressString.substr(0,tag);
        string indexString = addressString.substr(tag,index);

        //int tagValue = btoi(tagString);
        int indexValue = btoi(indexString);

        //The offset bits are simply ignored because it is enough if we look for the start address of the block

        hit h;

        //Initializing the hit object to 'false'
        //The address is searched for in the cache. If it found, it is set to true. If not it is simply returned as
        //it is without updating.

        h.isHit = false;
        h.hitWay = -1;

        //Search in each way of the set at the calculated index.
        for(int i=0; i<ways; i++){
            //The block 'current' is the block in focus for this iteration.
            //I'm assuming that the blocks are laid in set-first basis.
            block current = blocks[indexValue*ways+i];

            //The 'startAddress' variable of each block contains the address of the first block of the data it contains.
            //The tag can be extracted from this address.

            string currentTag = current.startAddress.to_string().substr(0,tag);
            if(current.isValid && currentTag==tagString){
                h.isHit = true;
                h.hitWay = i;
                break;
            }
        }
        return h;
    }


    //The write method writes into the cache at the passed address no matter what.
    //It returns information if the write has lead to an eviction or not.

    writeReturn Write(bitset<32> address){

        writeReturn w;

            string addressString = address.to_string();
            string tagString = addressString.substr(0,tag);
            string indexString = addressString.substr(tag,index);
            string offsetString = "";

            int indexValue = btoi(indexString);

            //The offset is set to 0. i.e. to address the first byte in the block.

            for(int i=0; i<offset; i++){
                offsetString = offsetString+"0";
            }


            addressString = tagString+indexString+offsetString;

            block temp;
            temp.isValid = true;
            temp.startAddress = bitset<32> (addressString);
            hit h = Read(address);
            //Check if the address to be written is present in the cache
            //If it is present just update the data.
            if(h.isHit){
                int hitWay = h.hitWay;
                blocks[indexValue*ways+hitWay] = temp;
                w.isEvicted = false;
                return w;
                //Should break.....

            }
            //Else Check if any of the blocks in the set is empty
            else{
                for(int i=0; i<ways; i++){
                        //If any of the blocks in the set is invalid, it means it is empty
                    if(!blocks[indexValue*ways+i].isValid){
                        blocks[indexValue*ways+i] = temp;
                        //There is an empty block in the set. Hence no need to evict data
                        w.isEvicted = false;
                        return w;
                    }
                }
            }
            //If both fail, the set is full and data needs to be evicted
            int evictionWay = counter[indexValue];
            block evictedBlock = blocks[indexValue*ways+evictionWay];
            blocks[indexValue*ways+evictionWay] = temp;
            //Increment round-robin counter
            if(evictionWay==ways-1){
                counter[indexValue] = 0;
            }
            else{
                counter[indexValue] = counter[indexValue]+1;
            }
            w.evictedBlock = evictedBlock;
            w.isEvicted = true;
            return w;
    }

};

int main(int argc, char* argv[]){



    config cacheconfig;
    ifstream cache_params;
    string dummyLine;
    cache_params.open(argv[1]);
    while(!cache_params.eof())  // read config file
    {
      cache_params>>dummyLine;
      cache_params>>cacheconfig.L1blocksize;
      cache_params>>cacheconfig.L1setsize;
      cache_params>>cacheconfig.L1size;
      cache_params>>dummyLine;
      cache_params>>cacheconfig.L2blocksize;
      cache_params>>cacheconfig.L2setsize;
      cache_params>>cacheconfig.L2size;
      }

   // Implement by you:
   // initialize the hirearch cache system with those configs
   // probably you may define a Cache class for L1 and L2, or any data structure you like

      cache L1Cache(cacheconfig.L1size, cacheconfig.L1setsize, cacheconfig.L1blocksize);
      cache L2Cache(cacheconfig.L2size, cacheconfig.L2setsize, cacheconfig.L2blocksize);


  int L1AcceState =0; // L1 access state variable, can be one of NA, RH, RM, WH, WM;
  int L2AcceState =0; // L2 access state variable, can be one of NA, RH, RM, WH, WM;


    ifstream traces;
    ofstream tracesout;
    string outname;
    outname = string(argv[2]) + ".out";

    traces.open(argv[2]);
    tracesout.open(outname.c_str());

    string line;
    string accesstype;  // the Read/Write access type from the memory trace;
    string xaddr;       // the address from the memory trace store in hex;
    unsigned int addr;  // the address from the memory trace store in unsigned int;
    bitset<32> accessaddr; // the address from the memory trace store in the bitset;

    if (traces.is_open()&&tracesout.is_open()){
        while (getline (traces,line)){   // read mem access file and access Cache

            istringstream iss(line);
            if (!(iss >> accesstype >> xaddr)) {break;}
            stringstream saddr(xaddr);
            saddr >> std::hex >> addr;
            accessaddr = bitset<32> (addr);


           // access the L1 and L2 Cache according to the trace;
              if (accesstype.compare("R")==0)

             {
                 //Implement by you:
                 // read access to the L1 Cache,
                 //  and then L2 (if required),
                 //  update the L1 and L2 access state variable;

                //Access L1
                if(L1Cache.Read(accessaddr).isHit){
                    L1AcceState = RH;
                    L2AcceState = NA;
                }
                //If value not found in L1, access L2
                else if(L2Cache.Read(accessaddr).isHit){
                    L1AcceState = RM;
                    L2AcceState = RH;
                    //Copy data to L1 on access
                    writeReturn returnedData = L1Cache.Write(accessaddr);
                    //If the write to L1 evicts data from L1, it needs to written into L2
                    if(returnedData.isEvicted){
                        //L2Cache.Write(returnedData.evictedBlock.startAddress);
                        //Check if the evicted data is present in L2
                        block evictedBlock = returnedData.evictedBlock;
                        if(L2Cache.Read(evictedBlock.startAddress).isHit){
                            //Equivalent to write-hit
                            //The data is present in L2. Hence it is updated.
                            //Since we are not modeling data, it is assumed that it is updated.
                        }
                        //If the data is not found in L2, we simply forward this write to main memory
                        //because the cache follows write no-allocate policy
                        else{
                            //Assumed that the write is forwarded to main-memory
                        }

                    }
                }
                //If not found in both, access main memory
                else{

                    L1AcceState = RM;
                    L2AcceState = RM;
                    //Copy data into L2
                    L2Cache.Write(accessaddr);      //If data is evicted for this write, it is assumed that it will
                                                    //written into main memory. So need to handle it
                    //Now copy the same data into L1
                    writeReturn returnedData = L1Cache.Write(accessaddr);
                    //If the write to L1 evicts data from L1, it needs to written into L2

                    if(returnedData.isEvicted){
                        //L2Cache.Write(returnedData.evictedBlock.startAddress);
                        //Check if the evicted data is present in L2
                        block evictedBlock = returnedData.evictedBlock;
                        if(L2Cache.Read(evictedBlock.startAddress).isHit){
                            //Equivalent to write-hit
                            //The data is present in L2. Hence it is updated.
                            //Since we are not modeling data, it is assumed that it is updated.
                        }
                        //If the data is not found in L2, we simply forward this write to main memory
                        //because the cache follows write no-allocate policy
                        else{
                            //Assumed that the write is forwarded to main-memory
                        }

                    }

                }






                 }
             else
             {
                   //Implement by you:
                  // write access to the L1 Cache,
                  //and then L2 (if required),
                  //update the L1 and L2 access state variable;

                    //Write no-allocate policy is used. Hence we just have to check if the address is present
                    //or not in the cache. If it is present, it is a hit. If not, a miss. There is no need to
                    //update the cache as it is the data that gets updated and we are not handling data here.

                    //Check if the address to be written is present in L1
                    if(L1Cache.Read(accessaddr).isHit){
                        //Write Hit in L1 !!!!
                        L1AcceState = WH;
                        //There is no need to access L2 now
                        L2AcceState = NA;
                    }
                    else if(L2Cache.Read(accessaddr).isHit){
                        //Write Miss in L1 and Hit in L2
                        L1AcceState = WM;
                        L2AcceState = WH;
                    }
                    //Miss in both L1 and L2. The write is simply forwarded to main memory
                    else{
                        L1AcceState = WM;
                        L2AcceState = WM;
                    }

                  }



            tracesout<< L1AcceState << " " << L2AcceState << endl;  // Output hit/miss results for L1 and L2 to the output file;


        }
        traces.close();
        tracesout.close();
    }
    else cout<< "Unable to open trace or traceout file ";







    return 0;
}

        int btoi(string s){
            int integerValue = 0;
            int length = s.size();
            int k;
            for(int i=0; i<length; i++){
                char c = s[(length-1)-i];
                if(c == '0')
                    k = 0;
                else
                    k = 1;
                integerValue = integerValue + k * pow(2,i);
            }
            return integerValue;
        }
