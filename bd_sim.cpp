
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <cmath>
#include <limits>
#include <cstdlib>
#include <algorithm>
#include <ctime>

using namespace std;

#include "bd_sim.h"
#include "tree.h"
#include "node.h"

BirthDeathSimulator::BirthDeathSimulator(double estop,double tstop, double brate, double drate):failures(0),
		maxfailures(1000),birthrate(brate),deathrate(drate),sumrate(brate+drate),relative_birth_rate(brate/(brate+drate)),
		extantstop(estop),timestop(tstop),numofchanges(0),currenttime(0.0),extantnodes(vector<Node*>()),
		BIRTHTIME(map<Node*,double>()),DEATHTIME(map<Node*,double>()){
	srand(time(0));
}

BirthDeathSimulator::BirthDeathSimulator():failures(0),maxfailures(1000),birthrate(0.1),deathrate(0.05),
		sumrate(0.1+0.05),relative_birth_rate(0.1/(0.1+0.05)),extantstop(10),timestop(0),numofchanges(0),
		currenttime(0.0),extantnodes(vector<Node*>()),BIRTHTIME(map<Node*,double>()),DEATHTIME(map<Node*,double>()){
	srand(time(0));
}

void BirthDeathSimulator::setup_parameters(){
	numofchanges = 0;
	currenttime = 0.0;
	extantnodes = vector<Node*>();
	BIRTHTIME = map<Node*,double>();
	DEATHTIME = map<Node*,double>();
}

Tree * BirthDeathSimulator::make_tree(bool show_dead){
	setup_parameters();
	root = new Node();
	BIRTHTIME[root] = currenttime;
	extantnodes.push_back(root);
	while(check_stop_conditions()){
		double dt = time_to_next_sp_event();
		currenttime += dt;
		event();
		if(extantnodes.size() < 1){
			failures += 1;
			setup_parameters();
			root = new Node();//need to clean
			BIRTHTIME[root] = currenttime;//need to clean
			extantnodes.push_back(root);//empty
		}
	}
	vector<Node*> temp_extant_nodes(extantnodes);
	for (unsigned int i=0;i<temp_extant_nodes.size();i++){
		try{
			DEATHTIME[temp_extant_nodes[i]];
			node_death(temp_extant_nodes[i]);
		}catch( char * str ){
			cout << "catch" << endl;
			node_death(temp_extant_nodes[i]);
			//temp_extant_nodes[i]->istip = 1
		}
	}
	root->setBL(0);
		double totallength = 0;
		int count = 1;
		tree = new Tree(root);
	if(show_dead == false){
		delete_dead_nodes();
	}

	for(int i=0;i<tree->getExternalNodeCount();i++){
		totallength += tree->getExternalNode(i)->getBL();
		std::stringstream out;
		out << count;
		tree->getExternalNode(i)->setName("taxon_"+out.str());
		count += 1;
	}
	return tree;
}

bool BirthDeathSimulator::check_stop_conditions(){
	bool keepgoing = true;
	if (extantstop > 0){
		if (extantnodes.size() >= extantstop){
			currenttime += time_to_next_sp_event();
			keepgoing = false;
		}
	}
	if(timestop > 0){
		if (currenttime >= timestop){
			currenttime = timestop;
			keepgoing = false;
		}
	}
	return keepgoing;
}

double BirthDeathSimulator::time_to_next_sp_event(){
	double num = rand() / double(RAND_MAX);
	return (-log(num))/ ( (extantnodes.size()) * sumrate);
}

void BirthDeathSimulator::event(){
	int random_integer = 0+int((extantnodes.size()-1)*rand()/(RAND_MAX + 1.0));
	Node * extant = extantnodes[random_integer];
	if (event_is_birth())
		node_birth(extant); // real speciation
	else
		node_death(extant);
}

void BirthDeathSimulator::node_death(Node *innode){
	DEATHTIME[innode] = currenttime;
	double bl = DEATHTIME[innode] - BIRTHTIME[innode];
	innode->setBL(bl);
	extantnodes.erase(find(extantnodes.begin(),extantnodes.end(),innode));
}

void BirthDeathSimulator::node_birth(Node *innode){
	Node * left = new Node();
	Node * right = new Node();
	innode->addChild(*left);
	innode->addChild(*right);
	BIRTHTIME[left] = currenttime;
	BIRTHTIME[right] = currenttime;
	node_death(innode);
	extantnodes.push_back(left);
	extantnodes.push_back(right);
}

void BirthDeathSimulator::delete_dead_nodes(){
	vector<Node *> kill;
	set_distance_to_tip();
	for (int i=0; i<tree->getExternalNodeCount();i++)
		if (get_distance_from_tip(tree->getExternalNode(i)) != root->getHeight())
			kill.push_back(tree->getExternalNode(i));
	for (unsigned int i=0;i<kill.size();i++)
		delete_a_node(kill[i]);
}

void BirthDeathSimulator::set_distance_to_tip(){
	for (int i=0;i<tree->getExternalNodeCount();i++){
		double curh = 0.0;
		tree->getExternalNode(i)->setHeight(curh);
		Node * tnode = tree->getExternalNode(i);
		while (tnode->hasParent()){
			curh += tnode->getBL();
			if (tnode->getHeight()<curh)
				tnode->setHeight(curh);
			tnode = tnode->getParent();
		}
		curh += tnode->getBL();
		if (tnode->getHeight()<curh)
			tnode->setHeight(curh);
	}
}

void BirthDeathSimulator::delete_a_node(Node * innode){
	Node * tparent = innode->getParent();
	if (tparent != root){
		Node * child = NULL;
		for (int i=0;i<tparent->getChildCount();i++)
			if (tparent->getChild(i)!= innode)
				child = tparent->getChild(i);
		Node * pparent = tparent->getParent();
		tparent->removeChild(*innode);
		tparent->removeChild(*child);
		pparent->removeChild(*tparent);
		pparent->addChild(*child);
		child->setParent(*pparent);
		child->setBL(child->getBL()+tparent->getBL());
	}else{
		Node * child = NULL;
		for (int i=0;i<tparent->getChildCount();i++)
			if (tparent->getChild(i) != innode)
				child = tparent->getChild(i);
		tparent->removeChild(*innode);
		tree->setRoot(child);
		root = child;
	}
}

bool BirthDeathSimulator::event_is_birth(){
	double x = rand() / double(RAND_MAX);
	if (x < relative_birth_rate)
		return true;
	else
		return false;
}

double BirthDeathSimulator::get_distance_from_tip(Node *innode){
	Node * cur = innode;
	double curh = 0.0;
    while (cur->hasParent()){
        curh += cur->getBL();
        cur = cur->getParent();
    }
    curh += cur->getBL();
    return curh;
}