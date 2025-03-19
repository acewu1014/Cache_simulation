#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
using namespace std;
#define DEBUG 0

string reference_list[4096];
string hit_or_miss[4096];


int main(int argc, char *argv[]) {
    if (argc != 4) {
        cout << "Please Input: project cache1.org reference1.lst index.rpt" << endl;
        return 0;
    }
    string config_file_name = argv[1];
    string input_file_name = argv[2];
    string otput_file_name = argv[3];
    fstream file;

//cache configuration setup

    file.open(config_file_name, ios::in);
    if (!file) {
        cerr << "Can't open the file: " << config_file_name << endl;
        exit(1);
    }

    string header;
    int data;
    int data_num = 0;
    int data_list[4] = {0};
    while(file >> header >> data) {
        data_list[data_num++] = data;
    }
    file.close();

    int address_bits = data_list[0];
    int block_size = data_list[1];
    int cache_sets = data_list[2];
    int associativity = data_list[3];
    int offset_bit_count = log2(block_size);
    int index_bit_count = log2(cache_sets);
    int index_bit[128] = {0}; //這邊感覺可以修改
    int tag_bit_count = address_bits - index_bit_count - offset_bit_count; 

    //目的只是存下使用index，用index bit的話要再改
    for(int i = offset_bit_count, j = 0; j < index_bit_count; ++i, ++j){
         index_bit[j] = i;
    }

//read reference list 

    file.open(input_file_name, ios::in);
    if (!file) {
        cerr << "Cannot Open File: " << input_file_name << endl;
        exit(1);
    }

    int ref_num = 0;
    bool already_cout_header = false;
    while(file >> header) {
        reference_list[ref_num] = header;
        if(!already_cout_header){
            file >> header;
            reference_list[ref_num] += " " + header;
            already_cout_header = true;
        }
#if(DEBUG)
        cout << "reference_list[" << ref_num << "] " << reference_list[ref_num] << endl;
#endif
        ++ref_num;
    }
    file.close();


//Zero Cost Indexing for Imporved Processor Cache Performance
//Tony Givargis
string o_ref[ref_num-2];
int M = address_bits - offset_bit_count;

//substract the number of header and end line
int num_ref_entry = ref_num-2;
//keep the essential part onlyn to calculate the correlation
for(int i = 1, j = 0; i < ref_num - 1; i++, j++){
    o_ref[j] = reference_list[i].substr(0, M);
} 

float correlation[M][M];
float quality[M];

/**************calculate correlation*****************/

for(int i = 0; i <M; i++){
    for (int j = i+1 ; j <M; j++){
        float E = 0;
        float D = 0;
        //calculate E(i,j)
        for(int k = 0; k < num_ref_entry; k++){
            if(o_ref[k][M-i-1]==o_ref[k][M-j-1]){
                E++;
            }
        }
        //calculate D(i,j)
        D = num_ref_entry - E;
        //calculate correlation
        correlation[i][j] = min(E, D)/max(E, D);
    }
} 
//cout<< correlation[0][1]<< endl;
/**********calculate quality measurment***********/

for(int i = 0; i <M; i++){
    float Z = 0;
    float O = 0;
    for(int k = 0; k < num_ref_entry; k++){        
        if(o_ref[k][M-i-1] == '0'){
            Z++;
        }
    }
    O = num_ref_entry-Z;
    quality[i] = min(Z, O)/max(Z, O);
}

/************Select the optimal index************/
int select_index[index_bit_count];
int select_tag[tag_bit_count];
for(int i = 0; i < index_bit_count; i++){

    // test index
    // for(int a = 0; a<M; a++){
    //     cout<< quality[a] << " ";
    // }
    // cout << endl;
    
    float max = -20;
    int index = -20;
    
    for(int i = M-1; i>=0; i--){
        if(quality[i]>max){
            max = quality[i];
            index = i;        
        }
    }
    //int index = int(distance(quality, max_element(quality, quality+M)));


    // cout << index << endl;
    quality[index] = -10;
    select_index[i] = index;
    for(int k = 0; k<M; k++){
        if(quality[k]>=0){
            quality[k] *= correlation[min(index, k)][std::max(index, k)];
        }
        else{
            quality[k] = -10;
        }
    }
}
//sort the selected index and add the offset 
sort(select_index, select_index+index_bit_count, greater<int>());
int temp = 0;
for(int i= M-1; i>=0; i--){
    int *it = find(select_index, select_index+index_bit_count, i);
    //i wasn't selected
    if(it==select_index+index_bit_count){        
        select_tag[temp] = i;
        temp++;
    } 
}

for(int i = 0; i< num_ref_entry; i++){
    cout << o_ref[i] << endl;
}

// for(int i = 0; i< index_bit_count; i++){
//     cout << select_index[i] << endl;
// }

//simualte the specified cache

    int miss_count = 0;
    //use to simaulate the cache
    string cache[cache_sets][associativity];

    //keep track of the info needed by LRU
    int LRU_list[cache_sets][associativity];

    //initialized the LRU list
    for(int i = 0; i < cache_sets; ++i)
    	for(int j = 0; j < associativity; ++j)
            LRU_list[i][j] = 0;

//simulate running on the reference list 

    for(int i = 1; i < ref_num - 1; ++i){
        string idxS = "";
        string tag = "";
        for (int j = 0; j<index_bit_count; j++){
            //cout << o_ref[i-1][select_index[j]] << endl;
            idxS += o_ref[i-1][M-select_index[j]-1];
        }
        for (int j = 0; j < tag_bit_count; j++){
            tag += o_ref[i-1][M-select_tag[j]-1];
        }
        /****************這邊要改index跟tag的取得方式****************/

        //tag = o_ref[i+1].substr(0, tag_bit_count);
        //idxS = o_ref[i+1].substr(tag_bit_count, index_bit_count);
        int idx = stoi(idxS, 0, 2);

#if(DEBUG)
        cout << "idx = " << idx << endl;
        cout << "---------" << endl;
#endif

        bool hit = false;
        for(int a = 0; a < associativity; ++a){
            if(cache[idx][a] == tag){
                hit_or_miss[i] = " hit";
                LRU_list[idx][a] = i;
                hit = true;
                break;
            }
        }
        if(hit){
            continue;
        }

        hit_or_miss[i] = " miss";
        miss_count++;
        int min = INFINITY;
        int index;
        //find the victim
        for(int a = 0; a < associativity; a++){
            if(LRU_list[idx][a]< min){
                min = LRU_list[idx][a];
                index = a;
            }
        }
        LRU_list[idx][index] = i;
        cache[idx][index] = tag;
    }
    
    
/********************* index.rpt **************************/
    file.open(otput_file_name, ios::out);
    if(!file) {
        cerr << "Cannot Open File: " << otput_file_name << endl;
        exit(1);
    }


    file << "Address bits: " << address_bits << endl;
    file << "Block size: " << block_size << endl;
    file << "Cache sets: " << cache_sets << endl;
    file << "Associativity: " << associativity << endl;
    file << endl;
    file << "Offset bit count: " << offset_bit_count << endl;
    file << "Indexing bit count: " << index_bit_count << endl;
    file << "Indexing bits:";
    for(int i = 0; i< index_bit_count; i++){
        file << " " << select_index[i]+offset_bit_count;
    }
    file << endl << endl;
    for(int i = 0; i < ref_num; ++i){
        file << reference_list[i] << hit_or_miss[i] << endl;
    }
    file << endl;
    file << "Total cache miss count: " << miss_count << endl;
    file.close();
    return 0;
}
