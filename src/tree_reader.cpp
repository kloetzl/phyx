/*
 * tree_reader.cpp
 *
 *  Created on: Nov 24, 2009
 *      Author: smitty
 */

#include <string>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <map>
#include <algorithm>

using namespace std;

#include "node.h"
#include "tree.h"
#include "tree_reader.h"
#include "utils.h"

TreeReader::TreeReader() {}

/*
 * the tree pointer coming in should just be a new Tree()
 * we should take this out as soon as we are ready to repoint
 * the existing code to the right bits
 */
Tree * TreeReader::readTree(string trees) {
    Tree * tree = new Tree();
    string pb = trees;
    unsigned int x = 0;
    char nextChar = pb.c_str()[x];
    bool start = true;
    bool keepGoing = true;
    bool in_quote = false;
    Node * currNode = NULL;
    while (keepGoing == true) {
	//cout << nextChar << endl;
        if (nextChar == '(') {
            if (start == true) {
                Node * root = new Node();
                tree->setRoot(root);
                currNode = root;
                start = false;
            } else {
                Node * newNode = new Node(currNode);
                currNode->addChild(*newNode);
                currNode = newNode;
            }
        } else if (nextChar == ',') {
            currNode = currNode->getParent();
        } else if (nextChar == ')') {
            currNode = currNode->getParent();
            x++;
            nextChar = pb.c_str()[x];
            string nam = "";
            bool goingName = true;
            if (nextChar == ',' || nextChar == ')' || nextChar == ':'
                || nextChar == ';'|| nextChar == '[') {
                goingName = false;
            }else if (nextChar == '"' || nextChar == '\''){
                in_quote = true;
            }
            while (goingName == true) {
                nam = nam + nextChar;
                x++;
                nextChar = pb.c_str()[x];
                if (in_quote == false){
                    if (nextChar == ',' || nextChar == ')' || nextChar == ':'
                    || nextChar == ';'|| nextChar == '[') {
                        goingName = false;
                        break;
                    }
                }else{
                   if (nextChar == '"' || nextChar == '\''){
                       goingName = false;
                       break;
                   }
                }
            } // work on edge
            currNode->setName(nam);
            if (in_quote == false){
                x--;
            }
        } else if (nextChar == ';') {
            keepGoing = false;
        } else if (nextChar == ':') {
            x++;
            nextChar = pb.c_str()[x];
            string edgeL = "";
            bool goingName = true;
            while (goingName == true) {
                edgeL = edgeL + nextChar;
                x++;
                nextChar = pb.c_str()[x];
                if (nextChar == ',' || nextChar == ')' || nextChar == ':'
                    || nextChar == ';'|| nextChar == '[') {
                    goingName = false;
                    break;
                }
            } // work on edge
            double edd = strtod(edgeL.c_str(), NULL);
            currNode->setBL(edd);
            x--;
        }
        //note
        else if (nextChar == '[') {
            x++;
            nextChar = pb.c_str()[x];
            string note = "";
            bool goingNote = true;
            while (goingNote == true) {
                note = note + nextChar;
                x++;
                nextChar = pb.c_str()[x];
                if (nextChar == ']' ) {
                    goingNote = false;
                    break;
                }
            }
            currNode->setComment(note);
        } else if (nextChar == ' ') {

        }
        // external named node
        else {
            Node * newNode = new Node(currNode);
            currNode->addChild(*newNode);
            currNode = newNode;
            string nodeName = "";
            bool goingName = true;
            if (nextChar == '"' || nextChar == '\''){
                in_quote = true;
            }
            if(in_quote == false){
                while (goingName == true) {
                    nodeName = nodeName + nextChar;
                    x++;
                    nextChar = pb.c_str()[x];
                    if (nextChar == ',' || nextChar == ')' || nextChar == ':' || nextChar == '[') {
                        goingName = false;
                        break;
                    }
                }
                x--;
            }else{
                x++;
                nextChar = pb.c_str()[x];
                while (goingName == true) {
                    nodeName = nodeName + nextChar;
                    x++;
                    nextChar = pb.c_str()[x];
                    if (nextChar == '"' || nextChar == '\''){
                        goingName = false;
                        break;
                    }
                } 
            }
            newNode->setName(nodeName);
        }
        if (x < pb.length() - 1) { // added
            x++;
        }
        nextChar = pb.c_str()[x];
    }
    tree->processRoot();
    return tree;
}

Tree * read_tree_string(string trees) {
    Tree * tree = new Tree();
    string pb = trees;
    unsigned int x = 0;
    char nextChar = pb.c_str()[x];
    bool start = true;
    bool keepGoing = true;
    bool in_quote = false;
    char quoteType;
    Node * currNode = NULL;
    while (keepGoing == true) {
        //cout << "Working on: " << nextChar << endl;
        if (nextChar == '(') {
            if (start == true) {
                Node * root = new Node();
                tree->setRoot(root);
                currNode = root;
                start = false;
            } else {
                Node * newNode = new Node(currNode);
                currNode->addChild(*newNode);
                currNode = newNode;
            }
        } else if (nextChar == ',') {
            currNode = currNode->getParent();
        } else if (nextChar == ')') {
            // internal named node
            currNode = currNode->getParent();
            x++;
            nextChar = pb.c_str()[x];
            string nodeName = "";
            bool goingName = true;
            in_quote = false;
            if (nextChar == ',' || nextChar == ')' || nextChar == ':'
                || nextChar == ';'|| nextChar == '[') {
                goingName = false;
            } else if (nextChar == '"' || nextChar == '\'') {
                in_quote = true;
                quoteType = nextChar;
            }
            while (goingName == true) {
                nodeName = nodeName + nextChar;
                x++;
                nextChar = pb.c_str()[x];
                if (in_quote == false) {
                    if (nextChar == ',' || nextChar == ')' || nextChar == ':'
                    || nextChar == ';'|| nextChar == '[') {
                        goingName = false;
                        break;
                    }
                } else {
                   if (nextChar == quoteType) {
                       //nodeName = nodeName + nextChar;
                       goingName = false;
                       break;
                   }
                }
            } // work on edge
            currNode->setName(nodeName);
            if (in_quote == false) {
                x--;
            }
        } else if (nextChar == ';') {
            keepGoing = false;
        } else if (nextChar == ':') {
            // edge length
            x++;
            nextChar = pb.c_str()[x];
            string edgeL = "";
            bool goingName = true;
            while (goingName == true) {
                edgeL = edgeL + nextChar;
                x++;
                nextChar = pb.c_str()[x];
                if (nextChar == ',' || nextChar == ')' || nextChar == ':'
                    || nextChar == ';'|| nextChar == '[') {
                    goingName = false;
                    break;
                }
            } // work on edge
            double edd = strtod(edgeL.c_str(), NULL);
            currNode->setBL(edd);
            x--;
        }
        // note/annotation
        else if (nextChar == '[') {
            x++;
            nextChar = pb.c_str()[x];
            string note = "";
            bool goingNote = true;
            while (goingNote == true) {
                note = note + nextChar;
                x++;
                nextChar = pb.c_str()[x];
                if (nextChar == ']' ) {
                    goingNote = false;
                    break;
                }
            }
            currNode->setComment(note);
        } else if (nextChar == ' ') {

        }
        // external named node
        else {
            Node * newNode = new Node(currNode);
            currNode->addChild(*newNode);
            currNode = newNode;
            string nodeName = "";
            bool goingName = true;
            in_quote = false;
            if (nextChar == '"' || nextChar == '\''){
                in_quote = true;
                quoteType = nextChar;
                nodeName = nodeName + nextChar;
            }
            if (in_quote == false) {
                while (goingName == true) {
                    nodeName = nodeName + nextChar;
                    x++;
                    nextChar = pb.c_str()[x];
                    if (nextChar == ',' || nextChar == ')' || nextChar == ':' || nextChar == '[') {
                        goingName = false;
                        break;
                    }
                }
                x--;
            } else {
                x++;
                nextChar = pb.c_str()[x];
                while (goingName == true) {
                    nodeName = nodeName + nextChar;
                    x++;
                    nextChar = pb.c_str()[x];
                    if (nextChar == quoteType) {
                        goingName = false;
                        nodeName = nodeName + nextChar;
                        break;
                    }
                } 
            }
            newNode->setName(nodeName);
        }
        if (x < pb.length() - 1) { // added
            x++;
        }
        nextChar = pb.c_str()[x];
    }
    tree->processRoot();
    return tree;
}

/*
 * tests the filetype by checking the first string and guessing based on
 * #nexus, ( newick
 * returns in the order above, 0 ,1 , 666 --- no filetype recognized
 * currently this only tests for nexus and newick
 *  if it is nexus, then the nexus reader will need 
 *  to deal with translate or not
 */
int test_tree_filetype(string filen) {
    string tline;
    ifstream infile(filen.c_str());
    int ret = 666; // if you get 666, there is no filetype set
    while (getline(infile,tline)) {
        if (tline.size() < 1) {
            continue;
        }
    //nexus
        if (tline[0] == '#') {
            ret = 0;
            break;
        }
    //newick
        if (tline[0] == '(') {
            ret = 1;
            break;
        }
        break;
    }
    infile.close();
    return ret;
}

/* tests the filetype by checking the first string and guessing based on 
 * #nexus, ( newick
 * returns in the order above, 0 ,1, 666 --- no filetype recognized
 */
int test_tree_filetype_stream(istream & stri, string & retstring) {
    if (!getline(stri, retstring)) {
        cout << "ERROR: end of file too soon" << endl;
    }
    int ret = 666; // if you get 666, there is no filetype set
    if (retstring[0] == '#') {
        ret = 0;
    } else if (retstring[0] == '(') {
        ret = 1;
    }
    return ret;
}

/**
 * this will look for the translation table if it exists
 */
bool get_nexus_translation_table(istream & stri, map<string, string> * trans,
    string * retstring) {
    string line1;
    string del(" \t");
    vector <string> tokens;
    bool exists = false; // is there a translation table?
    bool going = true;
    bool begintrees = false;
    bool tgoing = false;
    while (going) {
        if (!getline(stri, line1)) {
            break;
        }
        (*retstring) = line1;
        string uc(line1);
        transform(uc.begin(), uc.end(), uc.begin(), ::toupper);
        if (uc.find("TRANSLATE") != string::npos) {
            tgoing = true;
            exists = true;
            //cout << "Found translation table!" << endl;
            continue;
        } else if (begintrees == true && tgoing == false) {
            //cout << "No translation table present!" << endl;
                return false;
        }
        if (uc.find("BEGIN TREES") != string::npos) {
            begintrees = true;
           //cout << "Found Begin trees!" << endl;
        }
        if (tgoing == true) {
            //trimspaces and split up strings
            tokens.clear();
            tokenize(line1, tokens, del);
            size_t endcheck = line1.find(";");
            if (endcheck != string::npos) { // semicolon present. this is the last line.
                //cout << "Ending translation table!" << endl;
                going = false;
                (*retstring) = "";
            }
            if (tokens.size() != 1) { // not trailing lone semicolon
                for (unsigned int i=0; i < tokens.size(); i++) {
                    trim_spaces(tokens[i]);
                }
                size_t found = tokens[1].find(",");
                if (found != string::npos) {
                    tokens[1].erase(found, 1);
                }
                if (!going) {
                    size_t found2 = tokens[1].find(";");
                    if (found2 != string::npos) {
                        tokens[1].erase(found2, 1);
                    }
                }
                (*trans)[tokens[0]] = tokens[1];
                //cout << "tokens[0] = " << tokens[0] << "; tokens[1] = " << tokens[1] << endl;
            }
        }
    }
    return exists;
}


/*
 * this will read the nexus file after processing translating
 * should add some error correction code here
 */

Tree * read_next_tree_from_stream_nexus(istream & stri, string & retstring,
    bool ttexists, map<string,string> * trans, bool * going) {
    string tline;
    if (retstring.size() > 0) {
        tline = retstring;
        retstring = "";
    } else {
        bool reading = true; // continue reading if lines are empty. should check for comments too
        while (reading) {
            if (!getline(stri, tline)) {
                (*going) = false;
                return NULL;
            }
            //trim_spaces(tline);
            if (!tline.empty()) {
                reading = false;
            } else {
                //cout << "Skipping empty line" << endl;
            }
        }
    }
    //cout << "Working on: " << tline << endl;
    string uc(tline);
    transform(uc.begin(), uc.end(), uc.begin(), ::toupper);
    if (uc.find("END;") != string::npos) {
        (*going) = false;
        return NULL;
    }
    //vector<string> tokens;
    //string del(" \t");
    //tokenize(tline, tokens, del);
    //string tstring(tokens[tokens.size()-1]);
    
    size_t startpos = tline.find_first_of("(");
    string tstring = tline.substr(startpos);
    Tree * tree;
    //cout << tstring << endl;
    tree = read_tree_string(tstring);
    if (ttexists) {
        for (int i=0; i < tree->getExternalNodeCount(); i++) {
            tree->getExternalNode(i)->setName((*trans)[tree->getExternalNode(i)->getName()]);
        }
    }
    return tree;
}

/*
 * this is simple as each line is a tree
 */
// adding a simple check: if line is empty, assume we're done
Tree * read_next_tree_from_stream_newick(istream & stri, string & retstring, bool * going) {
    string tline;
    if (retstring.size() > 0) {
        tline = retstring;
        retstring = "";
    } else if (!getline(stri, tline)) {
        (*going) = false;
        return NULL;
    }
    if (tline.size() == 0) {
        //cout << "You've got yerself an empty line, there." << endl;
        (*going) = false;
        return NULL;
    }
    Tree * tree;
    tree = read_tree_string(tline);
    return tree;
}
