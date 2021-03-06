
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <getopt.h>
#include <algorithm>
#include <set>
using namespace std;

#include "tree.h"
#include "tree_reader.h"
#include "utils.h"
#include "log.h"

void print_help() {
    cout << "This will print out bipartitions found in treefile." << endl;
    cout << "Can read from stdin or file." << endl;
    cout << endl;
    cout << "Usage: pxbp [OPTION]... [FILE]..."<<endl;
    cout << endl;
    cout << " -t, --treef=FILE    input treefile, stdin otherwise" << endl;
    cout << " -v, --verbose       give more output" << endl;
    cout << " -e, --edgeall       force edgewise (not node) and assume all taxa are present in all trees" << endl;
    cout << " -u, --uniquetree    output unique trees as well as other output" << endl;
    cout << " -o, --outf=FILE     output file, stout otherwise" << endl;
    cout << " -h, --help          display this help and exit" << endl;
    cout << " -V, --version       display version and exit" << endl;
    cout << endl;
    cout << "Report bugs to: <https://github.com/FePhyFoFum/phyx/issues>" << endl;
    cout << "phyx home page: <https://github.com/FePhyFoFum/phyx>" << endl;
}
/*
 * add you name if you contribute (probably add another line)
 */
string versionline("pxbp 0.1\nCopyright (C) 2017 FePhyFoFum\nLicense GPLv3\nwritten by Stephen A. Smith (blackrim)");

static struct option const long_options[] =
{
    {"treef", required_argument, NULL, 't'},
    {"verbose", no_argument, NULL, 'v'},
    {"edgeall", no_argument, NULL, 'e'},
    {"uniquetree", no_argument, NULL, 'u'},
    {"outf", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {NULL, 0, NULL, 0}
};

int main(int argc, char * argv[]) {
    
    log_call(argc, argv);
    
    bool fileset = false;
    bool outfileset = false;
    bool verbose = false;
    bool edgewisealltaxa = false;
    bool uniquetree = false;
    char * treef = NULL;
    char * outf = NULL;
    while (1) {
        int oi = -1;
        int c = getopt_long(argc, argv, "t:o:veuhV", long_options, &oi);
        if (c == -1) {
            break;
        }
        switch(c) {
            case 't':
                fileset = true;
                treef = strdup(optarg);
                check_file_exists(treef);
                break;
            case 'v':
                verbose = true;
                break;
            case 'o':
                outfileset = true;
                outf = strdup(optarg);
                break;
            case 'e':
                edgewisealltaxa = true;
                break;
            case 'u':
                uniquetree = true;
                break;
            case 'h':
                print_help();
                exit(0);
            case 'V':
                cout << versionline << endl;
                exit(0);
            default:
                print_error(argv[0], (char)c);
                exit(0);
        }
    }
    
    istream* pios = NULL;
    ostream* poos = NULL;
    ifstream* fstr = NULL;
    ofstream* ofstr = NULL;
    
    if (fileset == true) {
        fstr = new ifstream(treef);
        pios = fstr;
    } else {
        pios = &cin;
        if (check_for_input_to_stream() == false){
            print_help();
            exit(1);
        }
    }
    if (outfileset == true) {
        ofstr = new ofstream(outf);
        poos = ofstr;
    } else {
        poos = &cout;
    }

    //read trees
    TreeReader tr;
    string retstring;
    int ft = test_tree_filetype_stream(*pios, retstring);
    if (ft != 0 && ft != 1) {
        cerr << "this really only works with nexus or newick" << endl;
        exit(0);
    }

    vector<Tree *> trees;
    bool going = true;
    if (ft == 0) {
        map<string,string> translation_table;
        bool ttexists;
        ttexists = get_nexus_translation_table(*pios, &translation_table, &retstring);;
        Tree * tree;
        while (going) {
            tree = read_next_tree_from_stream_nexus(*pios, retstring, ttexists,
                &translation_table, &going);
            if (tree != NULL) {
                trees.push_back(tree);
            }
        }
    } else if (ft == 1) {
        Tree * tree;
        while (going) {
            tree = read_next_tree_from_stream_newick(*pios, retstring, &going);
            if (going) {
                trees.push_back(tree);
            } 
        }
    }


    /*while (getline(*pios,retstring)) {
        if (retstring.size() < 3) {
            continue;
        }
        trees.push_back(tr.readTree(retstring));
    }*/

    int numtrees = trees.size();
    if (numtrees == 0) {
        if (fileset) {
            fstr->close();
            delete pios;
        }
        if (outfileset) {
            ofstr->close();
            delete poos;
        }
        cerr << "there are no trees;" << endl;
    }
    
    //get the biparts for the trees
    vector<string> names;
    set<string> names_s;
    map<string,int> name_index;
    map<int,string> name_st_index;
    //added to make sure we have all the names if it is partially overlapping
    for (unsigned int t = 0; t < trees.size(); t++) {
        for (int i=0; i < trees[t]->getExternalNodeCount(); i++) {
            string tname = trees[t]->getExternalNode(i)->getName();
            if (find(names.begin(), names.end(), tname) == names.end()) {
                name_index[tname] = i;
                names.push_back(tname);
                name_st_index[i] = tname;
            }
        }
    }
    copy(names.begin(),names.end(),inserter(names_s,names_s.begin()));

    vector<vector<int> > biparts; // first part of the bipart
    vector<vector<int> > biparts2; // second part of the bipart
    vector<vector<int> > not_included; // the names that aren't in the tree
    vector<double> bp_count;
    for (int i=0; i < numtrees; i++) {
        //get the biparts
        bool unrooted = false;
        int numch = trees[i]->getRoot()->getChildCount();
        if (numch > 2){
            unrooted = true;
        }
        vector<string> rt_nms = trees[i]->getRoot()->get_leave_names();
        set<string> rt_nms_set;
        copy(rt_nms.begin(),rt_nms.end(),inserter(rt_nms_set,rt_nms_set.begin()));
        //get the ones that aren't in the tree at all
        vector<string> not_included_nms(rt_nms_set.size());
        vector<int> not_included_i;
        vector<string>::iterator it2;
        it2 = set_difference(names_s.begin(),names_s.end(),rt_nms_set.begin(),rt_nms_set.end(),not_included_nms.begin());
        not_included_nms.resize(it2-not_included_nms.begin());
        for (unsigned int j=0; j < not_included_nms.size(); j++) {
            cerr <<" not included: "  << not_included_nms[j]<< endl;
            not_included_i.push_back(name_index[not_included_nms[j]]);
        }
        
        for (int j=0; j < trees[i]->getInternalNodeCount(); j++) {
            vector<string> nms = trees[i]->getInternalNode(j)->get_leave_names();
            //skip the root
            if (nms.size() == rt_nms.size()) {
                continue;
            }
            vector<int> nms_i;
            set<string> nms_s;
            copy(nms.begin(),nms.end(),inserter(nms_s,nms_s.begin()));
            for (unsigned int k=0; k < nms.size(); k++) {
                nms_i.push_back(name_index[nms[k]]);
            }
            sort(nms_i.begin(),nms_i.end());
            //get the other side of the bipart
            vector<int> nms_i2;
            vector<string> nms_s2(rt_nms.size());
            vector<string>::iterator it;
            it = set_difference(rt_nms_set.begin(), rt_nms_set.end(), nms_s.begin(),
                nms_s.end(), nms_s2.begin());
            nms_s2.resize(it-nms_s2.begin());
            for (unsigned int k=0; k < nms_s2.size(); k++) {
                nms_i2.push_back(name_index[nms_s2[k]]);
            }
            sort(nms_i2.begin(),nms_i2.end());
            //check to see if the bipart is new
            if( edgewisealltaxa == false){
                //this is nodewise and we dont' assume we have all the taxa
                if ((int)count(biparts.begin(), biparts.end(), nms_i) == 0 && 
                (int)count(biparts2.begin(), biparts2.end(), nms_i2) == 0) {
                    biparts.push_back(nms_i);
                    biparts2.push_back(nms_i2);
                    not_included.push_back(not_included_i);
                    bp_count.push_back(1);
                } else {
                    //get index 
                    size_t index = find(biparts.begin(),biparts.end(),nms_i)-biparts.begin();
                    bp_count[index] += 1;
                }
                /*
                 * do the otherside for unrooted
                 */
                if(unrooted==true && trees[i]->getInternalNode(j)->getParent()==trees[i]->getRoot()){
                    if ((int)count(biparts.begin(), biparts.end(), nms_i2) == 0 && 
                        (int)count(biparts2.begin(), biparts2.end(), nms_i) == 0) {
                        biparts.push_back(nms_i2);
                        biparts2.push_back(nms_i);
                        not_included.push_back(not_included_i);
                        bp_count.push_back(1);
                    }else{
                        size_t index = find(biparts.begin(),biparts.end(),nms_i2)-biparts.begin();
                        bp_count[index] += 1;
                    }
                }
            }else{
                //this is edgewise and we assume all the taxa 
                //this is for reporting a b | c d instead of a b and c d separately
                if ((int)count(biparts.begin(), biparts.end(), nms_i) == 0 && 
                    (int)count(biparts.begin(), biparts.end(), nms_i2) == 0){
                    biparts.push_back(nms_i);
                    biparts2.push_back(nms_i2);
                    not_included.push_back(not_included_i);
                    bp_count.push_back(1);
                } else {
                    //get index 
                    size_t index;
                    if ((int)count(biparts.begin(), biparts.end(), nms_i) == 1) {
                        index = find(biparts.begin(),biparts.end(),nms_i)-biparts.begin();
                    }else{
                        index = find(biparts.begin(),biparts.end(),nms_i2)-biparts.begin();
                    }
                    bp_count[index] += 1;
                }

            }
        }
    }
    
    (*poos) << numtrees << " trees " <<  endl;
    (*poos) << biparts.size() << " unique clades found" << endl;
    //calculate the logical matrix of biparts for each tree
    //the matrix will have each i as a tree and 
    //   each matrix[i] will represent a 1 if the tree has the bipart and 0 if not
    vector<int> cols(biparts.size()+1, 0);
    vector<vector<int> > matrix (numtrees, cols);
    for (int i=0; i < numtrees; i++) {
        bool unrooted = false;
        int numch = trees[i]->getRoot()->getChildCount();
        if (numch > 2){
            unrooted = true;
        }
        for (int j=0; j < trees[i]->getInternalNodeCount(); j++) {
            vector<string> nms = trees[i]->getInternalNode(j)->get_leave_names();
            vector<int> nms_i;
            for (unsigned int k=0; k < nms.size(); k++) {
                nms_i.push_back(name_index[nms[k]]);
            }
            sort(nms_i.begin(), nms_i.end());
            size_t index;
            if(edgewisealltaxa == true){
                if ((int)count(biparts.begin(), biparts.end(), nms_i) == 1) {
                    index = find(biparts.begin(),biparts.end(),nms_i)-biparts.begin();
                }else{
                    index = find(biparts2.begin(),biparts2.end(),nms_i)-biparts2.begin();
                } 
            }else{
                index = find(biparts.begin(), biparts.end(), nms_i) - biparts.begin();
            }
            matrix[i][index] = 1;
            if(unrooted==true && trees[i]->getInternalNode(j)->getParent()==trees[i]->getRoot()){
                vector<string> rt_nms = trees[i]->getRoot()->get_leave_names();
                set<string> rt_nms_set;
                copy(rt_nms.begin(),rt_nms.end(),inserter(rt_nms_set,rt_nms_set.begin()));
                set<string> nms_s;
                copy(nms.begin(),nms.end(),inserter(nms_s,nms_s.begin()));
                vector<int> nms_i2;
                vector<string> nms_s2(rt_nms.size());
                vector<string>::iterator it;
                it = set_difference(rt_nms_set.begin(), rt_nms_set.end(), nms_s.begin(),nms_s.end(), nms_s2.begin());
                nms_s2.resize(it-nms_s2.begin());
                for (unsigned int k=0; k < nms_s2.size(); k++) {
                    nms_i2.push_back(name_index[nms_s2[k]]);
                }
                sort(nms_i2.begin(),nms_i2.end());
                matrix[i][find(biparts.begin(), biparts.end(), nms_i2) - biparts.begin()] = 1;
            }
        }
    }
    /*
     * print the unique trees
     */
    if (uniquetree == true){
        cout << "====UNIQUE TREES====" << endl;
        set<string> un_trees;
        for (int i=0; i <numtrees; i++){
            string sv = get_string_vector(matrix[i]);
            if (count(un_trees.begin(),un_trees.end(),sv)==0){
                cout << trees[i]->getRoot()->getNewick(false) <<";" << endl;
                un_trees.insert(sv);
            }
        }
        cout << "==END UNIQUE TREES==" << endl;
    }

    //constructing the logical matrix
    //the logical matrix has each row as a bipart1 and each col as a name
    //there is a one if the bipart has the name
    //need to add the -1 for bipart2
    vector<int> cols2(names.size(),0);
    vector<vector<int> > logical_matrix (biparts.size(),cols2);
    for (unsigned int i=0; i < biparts.size(); i++) {
        for (unsigned int j=0; j < names.size(); j++) {
            if (count(biparts[i].begin(), biparts[i].end(), name_index[names[j]]) != 0) {
                logical_matrix[i][j] = 1;
            }
        }
        //cout << get_string_vector(logical_matrix[i]) << endl;
    }
    
    double smallest_proportion = 0.0;
    double TSCA = 0;
    //get the conflicting bipartitions
    //initialize results vectors
    for (unsigned int i = 0; i < biparts.size(); i++) {
        unsigned int sumc = sum_matrix_col(matrix,i);
        if (sumc != trees.size() && sumc > (smallest_proportion*trees.size())) {
            vector<string> nms;
            for (unsigned int k=0; k < biparts[i].size(); k++) {
                nms.push_back(name_st_index[biparts[i][k]]);
            }
            (*poos) << "CLADE: " << get_string_vector(nms);
            if(edgewisealltaxa == true){
                vector<string> nms_o;
                for (unsigned int k=0; k < biparts2[i].size(); k++) {
                    nms_o.push_back(name_st_index[biparts2[i][k]]);
                }
                (*poos) << "| " << get_string_vector(nms_o);
            }
            double totalcount = bp_count[i];
            vector<double> conflict_nums;
            conflict_nums.push_back(bp_count[i]);
            if (verbose)
                (*poos) << "\n\tCONFLICTS:" << endl;
            for (unsigned int j=0; j < biparts.size(); j++) {
                unsigned int sumc2 = sum_matrix_col(matrix,j);
                if (i != j && sumc2 != trees.size() && sumc2 > (smallest_proportion*trees.size())) {
                    bool logitest = test_logical(logical_matrix[i],logical_matrix[j]);
                    if (logitest) {
                        vector<string> nms2;
                        for (unsigned int k=0; k < biparts[j].size(); k++) {
                            nms2.push_back(name_st_index[biparts[j][k]]);
                        }    
                        totalcount += bp_count[j];
                        conflict_nums.push_back(bp_count[j]);
                        if(verbose){
                            (*poos) << " \t "<< get_string_vector(nms2);
                            if(edgewisealltaxa == true){
                                vector<string> nms_o;
                                for (unsigned int k=0; k < biparts2[j].size(); k++) {
                                    nms_o.push_back(name_st_index[biparts2[j][k]]);
                                }
                                (*poos) << "| " << get_string_vector(nms_o);
                            }
                            (*poos) << "\tCOUNT:\t" << bp_count[j] << "\tTREEFREQ:\t" << bp_count[j]/trees.size() <<  endl;
                        }
                    }
                }
            }
            //calculate ICA
            double sign = 1;
            for (unsigned int j=0; j < conflict_nums.size(); j++) {
                conflict_nums[j] /= totalcount;
                if (conflict_nums[j] > conflict_nums[0]) {
                    sign = -1;
                }
            }
            double ICA = 1;//same as logn(conflict_nums.size(),conflict_nums.size());
            for (unsigned int j=0; j < conflict_nums.size(); j++) {
                ICA += (conflict_nums[j] * logn(conflict_nums[j], conflict_nums.size()));
            }
            TSCA += ICA;
            ICA *= sign;
            (*poos) << "\tFREQ:\t"<< conflict_nums[0] << "\tICA:\t" << ICA << "\tCOUNT:\t" << bp_count[i] << "\tTREEFREQ:\t" << bp_count[i]/trees.size() <<  endl;
            if(verbose){
                (*poos) << "\tTREES:\t";
                for (int j=0;j<matrix.size();j++){
                    if (matrix[j][i] == 1){
                        (*poos) << j << " ";   
                    }
                }
                (*poos) << endl;
            }
        } else if (sumc == trees.size()) {
            vector<string> nms;
            for (unsigned int k=0; k < biparts[i].size(); k++) {
                nms.push_back(name_st_index[biparts[i][k]]);
            }
            (*poos) << "CLADE: " <<  get_string_vector(nms);
            if(edgewisealltaxa == true){
                vector<string> nms_o;
                for (unsigned int k=0; k < biparts2[i].size(); k++) {
                    nms_o.push_back(name_st_index[biparts2[i][k]]);
                }
                (*poos) << "| " << get_string_vector(nms_o);
            }
            (*poos) << "\tFREQ:\t1.\tICA:\t1.\tCOUNT:\t" << bp_count[i] << "\tTREEFREQ:\t1." << endl;
            TSCA += 1;
            if(verbose){
                (*poos) << "\tTREES:\t";
                for (int j=0;j<matrix.size();j++){
                    if (matrix[j][i] == 1){
                        (*poos) << j << " ";   
                    }
                }
                (*poos) << endl;
            }
        }
    }
    (*poos) << "TSCA: " << TSCA << endl;

    //shut things down
    if (fileset) {
        fstr->close();
        delete pios;
    }
    if (outfileset) {
        ofstr->close();
        delete poos;
    }
    return EXIT_SUCCESS;
}
