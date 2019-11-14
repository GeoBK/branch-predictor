#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
using namespace std;

#define SYSERR -1;
unsigned int call_count;
class Block {
    public:    
    bool is_dirty=false;
    bool is_valid=false;
    unsigned int tag_id;
    unsigned int timestamp;
};

bool CompareTimestamp(Block a, Block b){
    return a.timestamp>b.timestamp;
}

class Set{
    public:
    Set(int associativity){
        for(int i=0;i<associativity;i++){
            blocks.push_back(Block());
        }
    }
    vector<Block> blocks;
};
class Cache {
    public:
    int block_size;
    int cache_size;        
    int assoc;
    Cache* next_level;
    vector<Set> sets;

    int num_reads           =   0;
    int num_writes          =   0;
    int num_read_miss       =   0;
    int num_write_miss      =   0;
    int num_write_backs     =   0;
    int num_memory_access   =   0;

    int number_of_sets      =   0;
    int number_of_blocks    =   0;
    
    Cache(int block_size, int cache_size, int associativity): 
        block_size(block_size), cache_size(cache_size), assoc(associativity){
            number_of_blocks=cache_size/block_size;
            number_of_sets=number_of_blocks/associativity;
            for(int i=0;i<number_of_sets;i++){
                sets.push_back(Set(associativity));
            }            
        }
    void DisplayStats(){        
        cout<<"===== Simulation Results =====\n";
        cout<<"a. number of L1 reads:			"<<num_reads<<"\n";
        cout<<"b. number of L1 read misses:		"<<num_read_miss<<"\n";
        cout<<"c. number of L1 writes:			"<<num_writes<<"\n";
        cout<<"d. number of L1 write misses:		"<<num_write_miss<<"\n";
        printf("e. L1 miss rate:			%1.4f\n",((float)num_read_miss+(float)num_write_miss)/((float)num_reads+(float)num_writes));        
        cout<<"f. number of writebacks from L1 memory:	"<<num_write_backs<<"\n";
        cout<<"g. total memory traffic:		"<<num_memory_access<<"\n";
    }

    bool ReadFromAddress(unsigned int address){        
        int set_index = (address/block_size)%number_of_sets;        
        unsigned int tag = address/((unsigned int)number_of_sets*(unsigned int)block_size);
           
        num_reads++;        
        bool is_found=false;
        bool invalid_block_found=false;
        vector<Block>::reverse_iterator lru_block=sets[set_index].blocks.rbegin();
        vector<Block>::reverse_iterator invalid_block;
        for(vector<Block>::reverse_iterator it=sets[set_index].blocks.rbegin();it != sets[set_index].blocks.rend();it++){
            if(it->is_valid && it->tag_id == tag){
                is_found=true;
                it->timestamp=call_count++;
            }
            if(it->timestamp < lru_block->timestamp){
                lru_block=it;
            }
            if(!it->is_valid && !invalid_block_found){
                invalid_block=it;
                invalid_block_found=true;
            }
        }
        if(!is_found){

            num_read_miss++;
            num_memory_access++;

            
            if(invalid_block_found){
                invalid_block->tag_id=tag;
                invalid_block->is_valid=true;
                invalid_block->timestamp=call_count++;
                invalid_block->is_dirty=false;
            }else{
                if(lru_block->is_dirty){
                    //write back to lower level cache
                    num_write_backs++;
                    num_memory_access++;
                }
                lru_block->tag_id=tag;
                lru_block->is_valid=true;
                lru_block->timestamp=call_count++;
                lru_block->is_dirty=false;
            }
            
        }
        // printf("------------------------------------------------\n");
        // printf("# %d: read %0x\n",line,address);
        // printf("L1 read: %0x (tag %0x, index %d)\n", address, tag, set_index);
        // if(is_found){
        //     printf("L1 hit \n");
        // }else{
        //     printf("L1 miss \n");
        // }        
        // printf("L1 update LRU\n");
        return is_found;
                
    }
    bool WriteToAddress(unsigned int address){
        num_writes++;
        int set_index = (address/block_size)%number_of_sets;        
        unsigned int tag = address/((unsigned int)number_of_sets*(unsigned int)block_size);
                
        bool is_found=false;
        bool invalid_block_found=false;
        vector<Block>::reverse_iterator lru_block = sets[set_index].blocks.rbegin();
        vector<Block>::reverse_iterator invalid_block;
        for(vector<Block>::reverse_iterator it=sets[set_index].blocks.rbegin();it != sets[set_index].blocks.rend();it++){
            if(it->is_valid && it->tag_id == tag){
                is_found=true;
                it->timestamp=call_count++;
                it->is_dirty=true;
            }
            if(it->timestamp < lru_block->timestamp){
                lru_block=it;
            }
            if(!it->is_valid && !invalid_block_found){
                invalid_block=it;
                invalid_block_found=true;
            }
        }
        if(!is_found){

            num_write_miss++;
            //get block value from lower level memory
            //
            num_memory_access++;

            
            if(invalid_block_found){
                invalid_block->tag_id=tag;
                invalid_block->is_valid=true;
                invalid_block->timestamp=call_count++;
                invalid_block->is_dirty=true;
            }else{
                if(lru_block->is_dirty){
                    //write back to lower level cache
                    num_write_backs++;
                    num_memory_access++;
                }
                lru_block->tag_id=tag;
                lru_block->is_valid=true;
                lru_block->timestamp=call_count++;
                lru_block->is_dirty=true;
            }
            
        }
        // printf("------------------------------------------------\n");
        // printf("# %d: write %0x\n",line,address);
        // printf("L1 write: %0x (tag %0x, index %d)\n", address, tag, set_index);
        // if(is_found){
        //     printf("L1 hit \n");
        // }else{
        //     printf("L1 miss \n");
        // }   
        // printf("L1 set dirty\n");
        // printf("L1 update LRU\n");
        return true;
    }
    
    void PrintCacheContent(){        
        cout<<"FINAL BTB CONTENTS\n";
        int i=0;
        for(vector<Set>::iterator it=sets.begin();it != sets.end();it++){
            cout<<"set\t"<<i<<":";
            sort(it->blocks.begin(),it->blocks.end(),CompareTimestamp);
            for(vector<Block>::iterator bl_it=it->blocks.begin(); bl_it!=it->blocks.end(); bl_it++){                
                char dirty_char;
                if(bl_it->is_dirty){
                    dirty_char='D';
                }else{
                    dirty_char='N';
                }
                printf("\t%0x %c\t||",bl_it->tag_id,dirty_char);
                
            }
            cout<<"\n";
            i++;
        }

    }

};

unsigned int getAddress(string str)
{
    int space_index = str.find(" ");
    string addr_str = str.substr(0,space_index); 
    // cout<<addr_str<<"\n";
    return stoul(addr_str,nullptr,16);
}
char getExpectedPrediction(string str)
{
    return str[str.find(" ")+1];    
}

char decodePrediction(int prediction)
{
    if(prediction<2)
    {
        return 'n';
    }
    else
    {
        return 't';
    }
}

void increment(int &val)
{
    if(val!=3)
    {
        ++val;
    }
}

void decrement(int &val)
{
    if(val!=0)
    {
        --val;
    }
}
class BranchPredictor
{
    public:
    int num_predictions=0;
    int num_mispredictions=0;
    virtual char GetPredictions(string str)=0;
    virtual void UpdatePredictions(string str)=0;
    virtual void UpdateBhr(string str)=0;
    virtual void PrintCommand()=0;
    virtual void PrintConfig()=0;
    virtual void PrintContents()=0;

    float getMispredRate()
    {
        return ((float)num_mispredictions/(float)num_predictions)*100.00;
    }
};
class Bimodal:public BranchPredictor
{    
    int num_index_bits;
    int btb_size;
    int btb_assoc;
    string trace_file;
    vector<int> predictions;
    unsigned int getIndex(unsigned int addr)
    {
        addr=addr>>2;
        int index_mask = ~(0xFFFFFFFF<<num_index_bits);
        return (index_mask&addr);
    }
    public:
    Bimodal(int bimodal_index_bits, int size, int assoc, string trace)
    {
        btb_size=size;
        btb_assoc=assoc;
        num_index_bits=bimodal_index_bits;
        trace_file=trace;
        int bimodal_size = 1<<bimodal_index_bits;
        for(int i=0;i<bimodal_size;++i)predictions.push_back(2);        
    }
    char GetPredictions(string line)
    {
        num_predictions++;
        unsigned int addr= getAddress(line);
        char expected_prediction = getExpectedPrediction(line);
        int index = getIndex(addr);
        char prediction=decodePrediction(predictions[index]);
        if(expected_prediction != prediction)
        {            
            num_mispredictions++;
        }
        //Updating existing entry with taken branch
           
        return prediction;  
    }

    void UpdatePredictions(string line)
    {
        unsigned int addr= getAddress(line);
        char expected_prediction = getExpectedPrediction(line);
        int index = getIndex(addr);
        if(expected_prediction=='t')
        {
            increment(predictions[index]);
        }
        else
        {
            decrement(predictions[index]);
        }   
    }
    void UpdateBhr(string str)
    {
        //Do nothing
    }

    void PrintCommand()
    {
        cout<<"COMMAND\n";
        cout<<"./sim bimodal "<<num_index_bits<<" "<<btb_size<<" "<<btb_assoc<<" "<<trace_file<<"\n";
    }

    void PrintConfig()
    {
        cout<<"COMMAND\n";
        cout<<"./sim bimodal "<<num_index_bits<<" "<<btb_size<<" "<<btb_assoc<<" "<<trace_file<<"\n";
        cout<<"OUTPUT\n";
        cout<<"number of predictions: "<<num_predictions<<"\n";
        cout<<"number of mispredictions: "<<num_mispredictions<<"\n";
        printf("misprediction rate: %0.2f%%\n",getMispredRate());
    }
    
    void PrintContents()
    {        
        cout<<"FINAL BIMODAL CONTENTS\n";
        for(unsigned int i=0;i<predictions.size();++i)
        {
            cout<<i<<" "<<predictions[i]<<"\n";
        }
    }
};

class Gshare:public BranchPredictor
{
    unsigned int bhr=0;
    vector<int> predictions;
    
    int num_index_bits;
    int num_bhr_bits;
    int btb_size;
    int btb_assoc;
    string trace_file;    
    public:
    Gshare(int gshare_index_bits, int bhr_bits, int size, int assoc, string trace)
    {
        num_index_bits=gshare_index_bits;
        num_bhr_bits = bhr_bits;
        btb_size=size;
        btb_assoc=assoc;
        trace_file=trace;
        int table_size = 1<<num_index_bits;
        for(int i=0;i<table_size;++i)predictions.push_back(2); 
    }
    unsigned getIndex(unsigned int addr)
    {
        addr=addr>>(2);
        int hr=bhr<<(num_index_bits-num_bhr_bits);        
        int index_mask = ~(0xFFFFFFFF<<num_index_bits);
        addr=addr^hr;
        return (index_mask&addr);
    }

    void UpdateBhr(string line)
    {
        char br = getExpectedPrediction(line);
        bhr=bhr>>1;
        if(br=='t')
        {            
            bhr=bhr|(1<<(num_bhr_bits-1));
        }
        
    }

    char GetPredictions(string line)
    {
        num_predictions++;
        unsigned int addr= getAddress(line);
        char expected_prediction = getExpectedPrediction(line);
        int index = getIndex(addr);
        // int old_value=predictions[index];
        char prediction=decodePrediction(predictions[index]);
        if(expected_prediction != prediction)
        {            
            num_mispredictions++;
        }        
        
        return prediction;
        // cout<<"GSHARE index: "<<index<<" old value: "<<old_value<<" new value "<<predictions[index]<<"\n";
        // cout<<"BHR UPDATED:"<<bhr<<"\n";
    }

    void UpdatePredictions(string line)
    {
        unsigned int addr= getAddress(line);
        char expected_prediction = getExpectedPrediction(line);
        int index = getIndex(addr);
        //Updating existing entry with taken branch
        if(expected_prediction=='t')
        {
            increment(predictions[index]);
        }
        else
        {
            decrement(predictions[index]);
        }
    }    

    void PrintCommand()
    {
        cout<<"COMMAND\n";
        cout<<"./sim gshare "<<num_index_bits<<" "<<num_bhr_bits<<" "<<btb_size<<" "<<btb_assoc<<" "<<trace_file<<"\n";
    }

    void PrintConfig()
    {
        cout<<"COMMAND\n";
        cout<<"./sim gshare "<<num_index_bits<<" "<<num_bhr_bits<<" "<<btb_size<<" "<<btb_assoc<<" "<<trace_file<<"\n";
        cout<<"OUTPUT\n";
        cout<<"number of predictions: "<<num_predictions<<"\n";
        cout<<"number of mispredictions: "<<num_mispredictions<<"\n";
        printf("misprediction rate: %0.2f%%\n",getMispredRate());
    }
    void PrintContents()
    {        
        cout<<"FINAL GSHARE CONTENTS\n";
        for(unsigned int i=0;i<predictions.size();++i)
        {
            cout<<i<<" "<<predictions[i]<<"\n";
        }
    }
};

class Hybrid:public BranchPredictor
{
    vector<int> chooser;
    int num_chooser_bits;
    int num_gshare_index_bits;
    int num_bimodal_index_bits;
    int num_bhr_bits;
    int btb_size;
    int btb_assoc;
    string trace_file;
    Bimodal* bimodal;
    Gshare*  gshare;
    public:
    Hybrid(int chooser_table_bits,int gshare_index_bits, int bhr_bits, int bimodal_index_bits, int size, int assoc, string trace)
    {
        num_chooser_bits=chooser_table_bits;
        num_gshare_index_bits=gshare_index_bits;
        num_bhr_bits = bhr_bits;
        num_bimodal_index_bits=bimodal_index_bits;
        btb_size=size;
        btb_assoc=assoc;
        trace_file=trace;
        int table_size = 1<<chooser_table_bits;        
        for(int i=0;i<table_size;++i)chooser.push_back(1);
        bimodal=new Bimodal(bimodal_index_bits,size,assoc,trace);
        gshare=new Gshare(gshare_index_bits,bhr_bits,size,assoc,trace);
    }
    unsigned getIndex(unsigned int addr)
    {
        addr=addr>>(2);               
        int index_mask = ~(0xFFFFFFFF<<num_chooser_bits);        
        return (index_mask&addr);
    }

    void UpdateBhr(string line)
    {
        gshare->UpdateBhr(line);        
    }

    char GetPredictions(string line)
    {
        num_predictions++;
        unsigned int addr= getAddress(line);
        char expected_prediction = getExpectedPrediction(line);
        int index = getIndex(addr);
        int choice=chooser[index];
        char prediction;
        
        if(choice<2)
        {            
            prediction=bimodal->GetPredictions(line);
        }
        else
        {
            prediction = gshare->GetPredictions(line);
        }       
        if(expected_prediction != prediction)
        {            
            num_mispredictions++;
        }   
        return prediction;
        // cout<<"GSHARE index: "<<index<<" old value: "<<old_value<<" new value "<<predictions[index]<<"\n";
        // cout<<"BHR UPDATED:"<<bhr<<"\n";
    }

    void UpdatePredictions(string line)
    {
        unsigned int addr= getAddress(line);
        char expected_prediction = getExpectedPrediction(line);        
        char bimodal_prediction=bimodal->GetPredictions(line);
        char gshare_prediction = gshare->GetPredictions(line);
        int index = getIndex(addr);
        int choice=chooser[index];
        if(choice<2)
        {            
            bimodal->UpdatePredictions(line);
        }
        else
        {
            gshare->UpdatePredictions(line);
        }
        //Updating existing entry with taken branch
        if(bimodal_prediction==expected_prediction && gshare_prediction!=expected_prediction)
        {
            decrement(chooser[index]);
        }
        else if(gshare_prediction==expected_prediction && bimodal_prediction!=expected_prediction)
        {
            increment(chooser[index]);
        }
    }

    void PrintCommand()
    {
        cout<<"COMMAND\n";
        cout<<"./sim hybrid "<<num_chooser_bits<<" "<<num_gshare_index_bits<<" "<<num_bhr_bits<<" "<<num_bimodal_index_bits<<" "<<btb_size<<" "<<btb_assoc<<" "<<trace_file<<"\n";
    }

    void PrintConfig()
    {
        cout<<"COMMAND\n";
        cout<<"./sim hybrid "<<num_chooser_bits<<" "<<num_gshare_index_bits<<" "<<num_bhr_bits<<" "<<num_bimodal_index_bits<<" "<<btb_size<<" "<<btb_assoc<<" "<<trace_file<<"\n";
        cout<<"OUTPUT\n";
        cout<<"number of predictions: "<<num_predictions<<"\n";
        cout<<"number of mispredictions: "<<num_mispredictions<<"\n";
        printf("misprediction rate: %0.2f%%\n",getMispredRate());
    }

    void PrintContents()
    {        
        cout<<"FINAL CHOOSER CONTENTS\n";
        for(unsigned int i=0;i<chooser.size();++i)
        {
            cout<<i<<" "<<chooser[i]<<"\n";
        }
        gshare->PrintContents();
        bimodal->PrintContents();
    }

};

class BtbNode
{
    unsigned int tag_id;
    bool is_valid;
    unsigned int target;
    int branch_type;
};

class Btb
{
    vector<BtbNode> nodes;
};

int main(int argc, char* _argv[])
{
    if(argc < 1 ){
        cout<<"Improper Arguments...First argument needs to be one of (bimodal, gshare or hybrid)\n";        
        return SYSERR;
    }
    int bimodal_index_bits ;
    int gshare_index_bits;
    int ghr_bits;
    int btb_size=0;
    int btb_assoc=0;
    char* trace_file;   
    string mode =_argv[1];
    BranchPredictor *bp=NULL;
    if(mode=="bimodal")
    {
        if(argc!=6)
        {
            cout<<"Improper Arguments \n";
            cout<<"Example Usage: sim bimodal <M2> <BTB size> <BTB assoc> <tracefile> \n";
            return SYSERR;
        }
        bimodal_index_bits  =   atoi(_argv[2]);
        btb_size            =   atoi(_argv[3]);
        btb_assoc           =   atoi(_argv[4]);
        trace_file          =   _argv[5];
        
        bp = new Bimodal(bimodal_index_bits,btb_size,btb_assoc,trace_file);
    }
    else if(mode=="gshare")
    {
        if(argc!=7)
        {
            cout<<"Improper Arguments \n";
            cout<<"Example Usage: sim gshare <M1> <N> <BTB size> <BTB assoc> <tracefile> \n";
            return SYSERR;
        }
        gshare_index_bits   =   atoi(_argv[2]);
        ghr_bits            =   atoi(_argv[3]);
        btb_size            =   atoi(_argv[4]);
        btb_assoc           =   atoi(_argv[5]);
        trace_file          =   _argv[6];
        if(gshare_index_bits<ghr_bits)
        {
            cout<<"Number of index bits cant be greater than ghr_bits\n";
            return SYSERR;
        }
        bp = new Gshare(gshare_index_bits,ghr_bits,btb_size,btb_assoc,trace_file);
    }
    else if(mode=="hybrid")
    {
        if(argc!=9)
        {
            cout<<"Improper Arguments \n";
            cout<<"Example Usage: sim hybrid <K> <M1> <N> <M2> <BTB size> <BTB assoc> <tracefile> \n";
            return SYSERR;
        }
        int chooser_table_bits= atoi(_argv[2]);
        gshare_index_bits   =   atoi(_argv[3]);
        ghr_bits            =   atoi(_argv[4]);
        bimodal_index_bits  =   atoi(_argv[5]);
        btb_size            =   atoi(_argv[6]);
        btb_assoc           =   atoi(_argv[7]);
        trace_file          =   _argv[8];
        if(gshare_index_bits<ghr_bits)
        {
            cout<<"Number of ghr_bits cant be greater than index bits \n";
            return SYSERR;
        }
        bp = new Hybrid(chooser_table_bits,gshare_index_bits,ghr_bits,bimodal_index_bits,btb_size,btb_assoc,trace_file);
    }
    else
    {
        cout<<"Improper Arguments...First argument needs to be one of (bimodal, gshare or hybrid)\n";   
        return SYSERR;
    }
    

    ifstream infile;
    infile.open(trace_file);
    if(!infile.is_open()){
        cerr<<"Unable to open "<<trace_file<<"!!!";
        return SYSERR;
    }
    string str;
    int line=0;
    int btb_mispred=0;
    Cache *btb=NULL;
    if(btb_size!=0)
    {
        btb= new Cache(4,btb_size,btb_assoc);
    }    
    while(!infile.eof())
    {
        
               
        getline(infile,str);
        if(str!="")
        {
            line++; 
            if(btb_size!=0)
            {
                if(!btb->ReadFromAddress(getAddress(str)))
                {
                    if(getExpectedPrediction(str)=='t')
                    {
                        ++btb_mispred;
                    }
                    continue;
                }
            }
            // cout<<line<<". PC: "<<str<<"\n";
            
            bp->GetPredictions(str);
            bp->UpdatePredictions(str);
            bp->UpdateBhr(str);
        }
    }
    if(btb_size==0)
    {
        bp->PrintConfig();
        bp->PrintContents();
    }
    else
    {
        bp->PrintCommand();
        cout<<"OUTPUT\n";
        cout<<"size of BTB:	"<<btb->cache_size<<"\n";        
        cout<<"number of branches:	"<<line<<"\n";
        cout<<"number of predictions from branch predictor:	"<<bp->num_predictions<<"\n";
        cout<<"number of mispredictions from branch predictor:	"<<bp->num_mispredictions<<"\n";
        cout<<"number of branches miss in BTB and taken:	"<<btb_mispred<<"\n";
        float total_mispred = bp->num_mispredictions+btb_mispred;
        cout<<"total mispredictions:	"<<bp->num_mispredictions+btb_mispred<<"\n";
        printf("misprediction rate: 	%0.2f%%\n",(total_mispred/(float)line)*100.00);        
        btb->PrintCacheContent();
        bp->PrintContents();
    }
    
    

    return 0;
}