/*

ALGORITHM
If word is one letter long, add it under the root node for that letter.
If word is two letters long, add it as the second level directly.



*/

#include "utils.h"

#include <string>
#include <vector>

using namespace std;
using namespace utils;

static int nodeId = 0;

class Node {
public:
	//Node() : letter(0) {}
	Node(char c, short index) : letter(c), wordIndexes { index }, id {nodeId++} {};
	Node(char c) : letter(c), id {nodeId++} {};
	char letter;
	vector<Node*> children;
	vector<short> wordIndexes;
	int id;
};

int wi = 0;
Node root {' ', -1};

void insertWord2(const string &word, short index) {

	Node *currentNode = &root;
	Node *nextNode;
	//char lastLetter = 0;

	int count = 0;
	if(word.length() == 1)
		count = 1;
	for(auto c : word) {

		//printf("Letter '%c'\n", c);

		nextNode = nullptr;
		//printf("current node %d, kids %d\n", currentNode->id, currentNode->children.size());
		for(auto node : currentNode->children) {
			//printf("%c vs %c\n", node->letter, c);
			if(node->letter == c) {
				//printf("Updating node\n");
				if(count > 0) {
					node->wordIndexes.push_back(index);
					wi++;
				}
				nextNode = node;
				break;
			}
		}
		if(!nextNode) {
			//printf("New node\n");
			if(count > 0) {
				currentNode->children.push_back(new Node(c, index));
				wi++;
			} else {
				currentNode->children.push_back(new Node(c));
			}
			nextNode = currentNode->children.back();
		}
		
		count++;
		//lastLetter = currentNode->letter;
		currentNode = nextNode;
	}
}

void insertWord(const string &word, short index) {
	for(int i=0; i<word.length()-1; i++) {
		insertWord2(word.substr(i), index);
	}
}

static string spaces { "                                                        " };

void printTree(Node *n, int level = 0) {

	for(auto node : n->children) {
		//printf("[%d] %c has %d children\n", level, node->letter, node->wordIndexes.size());
		printf("%s'%c' (%d)\n", spaces.substr(0, level).c_str(), node->letter, node->children.size());
		printTree(node, level+1);
		//for(auto n2 : node->children) {
		//	printTree(n2, level+1);
		//}
	}
}




int main(int argc, char **argv) {
	File file { "words.txt" };

	vector<string> lines;

	for(auto l : file.getLines()) {
		l.pop_back();
		lines.push_back(l);
	}

	int i = 0;
	for(auto l : lines) {
		auto words = split(l, " ");
		printf("Adding '%s'\n", l.c_str());
		for(auto w : words) {
			insertWord(w, i);
		}
		i++;
	}
	//insertWord("delta", 0);
	//insertWord("spellbound", 0);
	//insertWord("space", 0);

	printTree(&root);
	printf("Created %d Nodes and %d indexes\n", nodeId, wi);

	string query = argv[1];

	Node *currentNode = &root;
	for(auto c : query) {
		bool found = false;
		for(auto node : currentNode->children) {
			if(node->letter == c) {
				if(node->wordIndexes.size() > 0) {
					printf("Found '%c' in line %d\n", c, node->wordIndexes[0]);
				} else
					printf("Found '%c'\n", c);
				found = true;
				currentNode = node;
				break;
			}
		}
		if(!found) {
			printf("Did not find '%c'\n", c);
			break;
		}
	}

	return 0;
}