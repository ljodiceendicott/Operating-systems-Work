/*******************************************************
 **  Author: XXXXXXXXXXXXXXX
 **
 **
 **  Original Author: Michael Ocean
 **          Endicott College
 **          mocean@endicott.edu
 **
 **  What is it?: Template for csc380
 **               Programming assignment #4
 **
 **  Description:
 **               simulates allocating blocks for requests using
 **                 various 'fit' algorithms
 **
 **
 **		GET SET UP (RPi ONLY): sudo apt-get install libsdl1.2-dev
 **
 **  Your task:
 **            implement missing functions: bestfit(), worstfit(),
 **            firstfit(), deleteEntry() and compactMemory()
 **
 **  compile with:
 **               make
 **             (or)  g++ fitsim_gfx.cpp gfx.c -o fitgfx -lX11 -lm
 **
 **  run with:
 **               ./fitgfx
 **
 **  or with:
 **
 **               ./fitgfx < sequence_file.txt
 **
 ******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <list>
#include "memory_interface.cpp"

using namespace std;

const char FIRST_FIT = 'F';
const char WORST_FIT = 'W';
const char BEST_FIT = 'B';

struct alloc_entry_t{
	int start;
	int length;
	char contents;
};

// GLOBALS
char nAllocatorType;
list<alloc_entry_t> free_list;
list<alloc_entry_t> used_list;

/**********
 TEMPLATES
 **********/
MemoryInterface mem;

string promptUser();
void deleteEntry(char ch);
void showhelp();
void compactMemory();
bool store(int, char);
/****
 END TEMPLATES
 ****/

//this is the compare that is used in the WorstFit
//this one organizes to go from largest -> smallest
bool largestSize(const alloc_entry_t& first,const alloc_entry_t& second){
	return first.length > second.length;
}
//this is the compare that is used in the bestFit
//this one organizes to go from smallest -> largest
bool smallestSize(const alloc_entry_t& first,const alloc_entry_t& second){
	return first.length < second.length;
}
//this is the compare that is used in the firstFit
//this one organizes to go from earliest -> latest
bool earliest(const alloc_entry_t& first,const alloc_entry_t& second){
	return first.start < second.start;
}

/**
 ** Finds the first free section of memory to fit 'length' many characters.
 ** --this function should return the location of the beginning of
 ** a sufficiently large hole.  Specifically, the first on big enough.
 Returns -1 if there isn't enough free memory.
 ** TODO: Finish this function!
**/
int firstfit(int requestedLength)
{
	free_list.sort(earliest);
    // NOTE: this file does NOT need to add the record to the used_list,
    // because store() takes care of that for you!
    for(list<alloc_entry_t>::iterator it = free_list.begin(); it != free_list.end(); it++)
    {
        if(it->length >= requestedLength){
            // can fit here
            int loc = it->start;
            if(it->length > requestedLength){
                it->length -= requestedLength;
                it->start += requestedLength;
            }
            else{
                free_list.erase(it);
            }
            return loc;
        }
    }


	// if you get here then I guess we couldn't find a big enough hole
	return -1;
}


/**
 ** TODO: IMPLEMENT THIS!!
 ** DESCRIPTION: find the `worst' hole for a length of
 ** size length (as defined by the worst-fit algorithm)
 ** and return that location.  Return -1 if no location
 ** exists.
 **
 */
int worstfit(int length)
{
free_list.sort(largestSize);
    // NOTE: this file does NOT need to add the record to the used_list,
    // because store() takes care of that for you!
    for(list<alloc_entry_t>::iterator it = free_list.begin(); it != free_list.end(); it++)
    {
        if(it->length >= length){
            // can fit here
            int loc = it->start;
            if(it->length > length){
                it->length -= length;
                it->start += length;
            }
            else{
                free_list.erase(it);
            }
            return loc;
        }
    }


	// if you get here then I guess we couldn't find a big enough hole
	return -1;
}


/**
 ** TODO: IMPLEMENT THIS!!
 ** DESCRIPTION: find the `best' hole for a length of
 ** size length (as defined by the worst-fit algorithm)
 ** and return that location.  Return -1 if no location
 ** exists.
 **
 */
int bestfit(int length)
{
	free_list.sort(smallestSize);
    // NOTE: this file does NOT need to add the record to the used_list,
    // because store() takes care of that for you!
    for(list<alloc_entry_t>::iterator it = free_list.begin(); it != free_list.end(); it++)
    {
        if(it->length >= length){
            // can fit here
            int loc = it->start;
            if(it->length > length){
                it->length -= length;
                it->start += length;
            }
            else{
                free_list.erase(it);
            }
            return loc;
        }
    }


	// if you get here then I guess we couldn't find a big enough hole
	return -1;
}


/**
 ** TODO: IMPLEMENT THIS!!
 ** DESCRIPTION: should compact the holes in the memory (i.e., defrag)
 **
 */
void compactMemory()
{
	//make one giant hole at the end of memory
	used_list.sort(earliest);
	free_list.sort(earliest);
	int length = 0;
	for(list<alloc_entry_t>::iterator it = used_list.begin(); it != used_list.end(); it++){
		mem.deleteContents(it->contents);
	}
	
	for(list<alloc_entry_t>::iterator it = used_list.begin(); it != used_list.end(); it++){
		//the used_list is in the right order
		if(it->start == length){
			//increase the length to the next location
			length = length + it->length;
		}
		//not in the right spot
		else{
			it->start = length;
			length = length + it->length;
		}
	}
	free_list.clear();
	// figure out the remaining size of the block
	int rem = mem.getSize();
	rem = rem - length;
	alloc_entry_t t;
	t.start = length;
	t.length = rem;
	free_list.insert(free_list.begin(),t);

	for(list<alloc_entry_t>::iterator it = used_list.begin(); it != used_list.end(); it++){
		mem.store(it->contents,it->start,it->length);
	}
	

}

void deleteEntry(char item){
    // TODO: anything you need to do in order to keep your
    // data structures consistent
	
	// find it in used_list
	list<alloc_entry_t>::iterator del;

	for(list<alloc_entry_t>::iterator it = used_list.begin(); it != used_list.end(); it++){
		if(it->contents == item){
			del = it;
			// delete from used_list
			used_list.erase(del);
			break;
		}
	}
	// add to holes list / coalesce the holes lists
	free_list.insert(free_list.end(),*del);
	
	list<alloc_entry_t>::iterator before;
	list<alloc_entry_t>::iterator after;

	bool foundBefore = false;
	bool foundAfter = false;

	int iterCount = 0;

	for(list<alloc_entry_t>::iterator it = free_list.begin(); it != free_list.end(); it++){
			before = next(it,-1);
			after = next(it, 1);
			if(iterCount > 0 && before->start + before->length == it->start){
				foundBefore = true;
			
			}
			if(iterCount < free_list.size() && it->start + it->length == after->start){
				//this is where the hole is found
				foundAfter = true;
			}
			
			iterCount++;
	}
	

	
	if(foundBefore && foundAfter){
		before->length = before->length + del->length + after->length;
		free_list.erase(del);
		
	}
	else if(foundBefore){
		before->length = before->length + del->length;
		free_list.erase(del);
	}
	else if(foundAfter){
		after->length = del->length+ after->length;
		after->start = del->start;
		free_list.erase(del);
	}
	free_list.sort(earliest);
	for(list<alloc_entry_t>::iterator it = free_list.begin(); it != free_list.end(); it++){
		if(it->start + it->length == next(it,1)->start){
			it->length = next(it,1)->length + it->length;
			free_list.erase(next(it,1));
		}
	}
	
    // this function MUST end by calling this method:
    mem.deleteContents(item);
}

//** this is for your debugging purposes and to give examples of navigating these structures **/
void debugAllocationTable(){
    //Print Holes and Files for debugging
    cout << "HOLES" << endl;
    for(list<alloc_entry_t>::iterator it = free_list.begin(); it != free_list.end(); it++)
    {
     cout << "Hole @" << (it)->start << " length: " << (it)->length << endl;
    }

    cout << "FILES" << endl;
    for(list<alloc_entry_t>::iterator it = used_list.begin(); it != used_list.end(); it++)
    {
     cout << "File '"<< (it)->contents << "' at " << (it)->start << " -length: " << (it)->length << endl;
    }
}


// DO NOT MODIFY!
int main (int argc,  char * argv[])
{
	int done = 0;
	string cmd; // our current command

    cout << "Please select the allocation method" << endl
	<< "  [F]irst-Fit " << endl
	<< "  [B]est-Fit  " << endl
	<< "  [W]orst-Fit " << endl << endl;

	cmd = promptUser();
	nAllocatorType = cmd[0];
	if((nAllocatorType!='F')&&(nAllocatorType!='B')&&(nAllocatorType!='W'))
		exit(-1);

	showhelp();

    // add the initial hole information to memory
    alloc_entry_t hole;
    hole.start = 0;
    hole.length = mem.getSize();
    hole.contents = '_';
    free_list.push_back(hole);

	while(!done)
	{
		// get input from the user
		cmd = promptUser();

		// if the user enters quit, then quit
		if(cmd.compare("quit")==0)
		{
			exit(0);
		}
		else if(cmd.compare("help")==0)
		{
			showhelp();
		}
		else if(cmd.compare("print")==0)
		{
            debugAllocationTable();
			mem.showUsage();
		}
		else if(cmd.compare("compact")==0)
		{
			compactMemory();
		}
		else if(cmd.compare("delete")==0)
		{
			cout << "content? (single character, please)\n";
			char content;
			cin >> content;
            deleteEntry(content);
			mem.showUsage();
		}
		else if(cmd.compare("add")==0)
		{
			cout << "content? (single character, please)\n";
			char content;
			cin >> content;

			cout << "length?\n";
			int length;
			cin >> length;

			cout << "About to store '"
				 << length << "' many instances of '"
				 << content << "'" << endl;

			if(store(length,content))
				mem.showUsage();
		}
		else
		{
			cout << "Ignoring unknown command.  Type 'help' for help \n";
		}

	}
}

/*****************************************
 SAFE TO IGNORE EVERYTHING BELOW THIS LINE
******************************************/
/**
 * DESCRIPTION: prompts user and gets a string
 * SAFE TO IGNORE.
 **/
string promptUser()
{
	string tmp;
	string line;
	// show a prompt
	cout << "$ ";

	// get input from the user
	cin >> line;

	return line;
}



/** shows help.
 * IGNORE
 **/
void showhelp()
{
	printf("quit\t\tquits the simulator\n");
	printf("print\t\tshows the current memory map\n");
	printf("delete\t\tdelete data from memory\n");
	printf("add\t\tadd new data to the memory\n");
	printf("compact\t\tcompacts the memory\n");
	printf("help\t\tshows this info\n");
}

/**
 * This function calls your fit function, and stores data into
 * the memory. DO NOT MODIFY!
 */
bool store(int len, char content)
{
    // find a fit of at least size len
    int loc = -1;

    if(nAllocatorType == FIRST_FIT)
       loc = firstfit(len);
    else if(nAllocatorType == WORST_FIT)
        loc = worstfit(len);
    else
        loc = bestfit(len);

    if(loc==-1)
    {
        cout << "Function said there was insufficient space to store data." << endl;
        return false;
    }
    cout << "Function recommended storing data at " << loc << "." << endl;

    // add the record to the allocation list
    alloc_entry_t new_file;
    new_file.start = loc;
    new_file.length = len;
    new_file.contents = content;
    used_list.push_back(new_file);

    // Actually write the contents into memory
    return mem.store(content, loc, len);
}

