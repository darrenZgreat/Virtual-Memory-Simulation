#include <iostream>
#include <vector>
#include <map>
#include <fstream>
using namespace std;

enum opCode {
	NEW=1, READ, WRITE, END 
};

enum replacementAlgorithm {
	FIFO, LRU, OPTIMAL
};

typedef struct {
	opCode opC;
	string parameter;
} command;

typedef struct {
	int frameNumber;
	bool dirty;
} frame;

typedef map<int, int> pageTable;

typedef vector<frame> frameTable;

typedef struct {
	bool valid;
	int size;
	pageTable pt;
} process;

const int PAGE_SIZE_FRAME_SIZE = 1000;
const int MEMORY_SIZE = 3000;
const int NUM_FRAMES = MEMORY_SIZE/PAGE_SIZE_FRAME_SIZE;

void processCommands(replacementAlgorithm algorithm, const vector<command> &c, int & totalProcesses, int &totalHits, int &totalFaults);
void processPageHit(replacementAlgorithm algorithm, frameTable & f, process & p, int page, string offset, int & numHits);
void processPageFault(frameTable & f, process & p, int page, string offset, int & numFaults);
void processPageFaultOPTIMAL(const vector<command> &c, int currentCommand, frameTable & f, process & p, int page, string offset, int & numFaults);
void allocateFreeFrame(frameTable & f, process & p, int page, string offset);
bool isDirty(frameTable & f, int frameNumber);
void setDirty(frameTable & f, int frameNumber);
void readInCommands(string filename, vector<command> & c);

int main(int argc, char *argv[]) 
{
	int totalProcesses, totalHits, totalFaults;
	vector<command> c;
	
	readInCommands("Vm.dat", c);
	
	cout << "*** First In First Out ***" << endl;
	processCommands(FIFO, c, totalProcesses, totalHits, totalFaults);
	cout << "####For all " << totalProcesses << " processes, total page hit is " << totalHits << ", total page fault is " << totalFaults << "####" << endl;
	
	cout << "*** Least Recently Used ***" << endl;
	processCommands(LRU, c, totalProcesses, totalHits, totalFaults);
	cout << "####For all " << totalProcesses << " processes, total page hit is " << totalHits << ", total page fault is " << totalFaults << "####" << endl;
	
	cout << "*** Optimal ***" << endl;
	processCommands(OPTIMAL, c, totalProcesses, totalHits, totalFaults);
	cout << "####For all " << totalProcesses << " processes, total page hit is " << totalHits << ", total page fault is " << totalFaults << "####" << endl;
}

/* Function:	processCommands
				int totalProcesses
				int totalHits;
				int totalFaults;
 *    Usage:	processCommands(FIFO, c, totalProcesses, totalHits, totalFaults);
 * -------------------------------------------
 * Prints the execution of the commands. 
 * - algorithm: the replacementAlgorithm to use
 * - c: contains the commands to execute
 * - totalProcesses: set to equal the total number of processes
 * - totalHits: set to equal the total number of page hits
 * - totalFaults: set to equal the total number of page faults
 */
void processCommands(replacementAlgorithm algorithm, const vector<command> &c, int & totalProcesses, int &totalHits, int &totalFaults)
{
	/*//!!!DEBUG CODE (prints the dirty frames)!!!!!!!!!!!!!!!!!!!!!!
	cout << "                                ";
	for(int i=0; i<f.size(); i++)
	{
		if(f[i].dirty) cout << f[i].frameNumber;
	}
	cout << endl;
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	
	process p;
	p.valid=false;
	frameTable f;
	totalProcesses = 0;
	totalHits = 0;
	totalFaults = 0;
	int numHits = 0; 
	int numFaults = 0;
	
	if(c[c.size()-1].opC != END)
	{
		cerr << "ERROR-- processCommands: The job is never ending. The last OpCode Must be opCode(4)."<< endl;
		exit(EXIT_FAILURE);
	}
	
	for(int i=0; i<c.size(); i++)
	{
		int code = c[i].opC;
		int param = stoi(c[i].parameter);
		string paramStr = c[i].parameter;
		if(code==NEW)
		{
			if(p.valid)
			{
				cerr << "ERROR-- processCommands: Only one job may be active; opCode(4) MUST precede an additional opCode(1)."<< endl;
				exit(EXIT_FAILURE);
			}
			else
			{
				cout << "New job, size " << paramStr << endl;
				p.size=param;
				p.valid=true;
				totalProcesses++;
			}
		}
		else if(code==READ || code==WRITE)
		{
			if(!p.valid)
			{
				cerr << "ERROR-- processCommands: There must be an active job to execute a Read/Write; opCode(1) MUST precede opCode(2) or opCode(3)."<< endl;
				exit(EXIT_FAILURE);
			}
			else
			{
				int page = param/PAGE_SIZE_FRAME_SIZE;
				string offset;
				/* format the offset to contain the appropriate number of leading 0's */
				int offsetLength = to_string(param%PAGE_SIZE_FRAME_SIZE).length();
				int offsetMaxLength = to_string(PAGE_SIZE_FRAME_SIZE-1).length();
				for(int j=offsetMaxLength-offsetLength; j>0; j--)
					offset += "0";
				offset += to_string(param%PAGE_SIZE_FRAME_SIZE);
				/**/
				
				if(code==READ)
				{
					cout << "Read " << paramStr << endl;
					if(param>=p.size)
						cout << "  Access Violation" << endl;
					else 
					{
						if(p.pt.count(page))
							processPageHit(algorithm, f, p, page, offset, numHits);
						else 
						{
							if(algorithm==OPTIMAL) processPageFaultOPTIMAL(c, i, f, p, page, offset, numFaults);
							else processPageFault(f, p, page, offset, numFaults);
						}	
					}
				}
				else if(code==WRITE)
				{
					cout << "Write " << paramStr << endl;
					if(param>p.size)
						cout << "  Access Violation" << endl;
					else 
					{
						if(p.pt.count(page))
							processPageHit(algorithm, f, p, page, offset, numHits);
						else 
						{
							if(algorithm==OPTIMAL) processPageFaultOPTIMAL(c, i, f, p, page, offset, numFaults);
							else processPageFault(f, p, page, offset, numFaults);
						}
						
						if(p.pt.count(page)) setDirty(f, p.pt[page]);
					}
				}
			}
		}
		else if(code==END)
		{
			if(!p.valid)
			{
				cerr << "ERROR-- processCommands: There is no active job to end; opCode(1) MUST precede opCode(4)."<< endl;
				exit(EXIT_FAILURE);
			}
			else 
			{
				cout << "End job" << endl;
				cout << "###Total page hit is " << numHits << "; total page fault is " << numFaults << "###" << endl;
				if(i<c.size()-1) cout << endl;
				totalHits+=numHits;
				totalFaults+=numFaults;
				numHits=0;
				numFaults=0;
				p.valid = false;
				p.pt.clear();
				f.clear();
			}
		}
	}
}

/* Function:	processPageHit
 *    Usage:	if(p.pt.count(page))
 *					processPageHit(p, page, offset, numHits);
 * -------------------------------------------
 * Process the page hit, prints the details.
 */
void processPageHit(replacementAlgorithm algorithm, frameTable & f, process & p, int page, string offset, int & numHits)
{
	if(!p.pt.count(page))
	{
		cerr << "ERROR-- processPageHit: page '" << page << "' is not in memory." << endl;
		exit(EXIT_FAILURE);
	}
	cout << "  Page hit" << endl;
	numHits++;
	cout << "  Location " << p.pt[page] << offset << endl;
	
	if(algorithm==LRU)
	{
		for(int i=0; i<f.size(); i++)
		{
			if(f[i].frameNumber == p.pt[page])
			{
				frame tempFrame;
				tempFrame.frameNumber = f[i].frameNumber;
				tempFrame.dirty = f[i].dirty;
				f.erase(f.begin()+i);
				f.push_back(tempFrame);
				break;
			}
		}
	}
}

/* Function:	processPageFault
 *    Usage:	if(!p.pt.count(page))
 *					processPageFault(f, p, page, offset, numFaults);
 * -------------------------------------------
 * Process the page fault, prints the details
 */
void processPageFault(frameTable & f, process & p, int page, string offset, int & numFaults)
{
	if(p.pt.count(page))
	{
		cerr << "ERROR-- processPageFault: page '" << page << "' is already in memory." << endl;
		exit(EXIT_FAILURE);
	}
	cout << "  Page fault " << endl;
	numFaults++;
	if(f.size()<NUM_FRAMES)
	{
		allocateFreeFrame(f, p, page, offset);
	}
	else
	{
		cout << "    Page replacement" << endl;
		if(isDirty(f, f.front().frameNumber))
			cout << "      Page out" << endl;
		for(pageTable::iterator it = p.pt.begin(); it!=p.pt.end(); ++it)
		{
			if(it->second==f.front().frameNumber) 
			{
				p.pt.erase(it->first);
				break;
			}
		}
		frame tempFrame;
		tempFrame.frameNumber = f.front().frameNumber;
		tempFrame.dirty = false;
		f.erase(f.begin());
		f.push_back(tempFrame);
		p.pt[page] = tempFrame.frameNumber;
		cout << "  Location " << p.pt[page] << offset << endl;
	}
}

/* Function:	processPageFaultOPTIMAL
 *    Usage:	if(!p.pt.count(page))
 *					processPageFaultOPTIMAL(f, p, page, offset, numFaults);
 * -------------------------------------------
 * Process the page fault, prints the details
 */
void processPageFaultOPTIMAL(const vector<command> &c, int currentCommand, frameTable & f, process & p, int page, string offset, int & numFaults)
{
	if(p.pt.count(page))
	{
		cerr << "ERROR-- processPageFault: page '" << page << "' is already in memory." << endl;
		exit(EXIT_FAILURE);
	}
	cout << "  Page fault" << endl;
	numFaults++;
	if(f.size()<NUM_FRAMES)
	{
		allocateFreeFrame(f, p, page, offset);
	}
	else
	{
		cout << "    Page replacement" << endl;
		int usedCount=0;
		vector<bool> recentlyUsed(NUM_FRAMES,false);
		for(int i=currentCommand+1; i<c.size() && usedCount<recentlyUsed.size()-1 && c[i].opC!=END; i++)
		{
			int nextPage = stoi(c[i].parameter)/PAGE_SIZE_FRAME_SIZE;
			if(p.pt.count(nextPage))
			{
				recentlyUsed[p.pt[nextPage]] = true; 
				usedCount++;
			}
		}
		int frameToReplace=0;
		for(int i=0; i<recentlyUsed.size(); i++)
		{
			if(recentlyUsed[i]==false)
			{
				frameToReplace=i;
				break;
			}
		}
		
		if(isDirty(f, f[frameToReplace].frameNumber))
			cout << "      Page out" << endl;
		for(pageTable::iterator it = p.pt.begin(); it!=p.pt.end(); ++it)
		{
			if(it->second==f[frameToReplace].frameNumber) 
			{
				p.pt.erase(it->first);
				break;
			}
		}	
		p.pt[page] = f[frameToReplace].frameNumber;
		cout << "  Location " << p.pt[page] << offset << endl;
	}
}

/* Function:	allocateFreeFrame
 *    Usage:	if(f.size()>=NUM_FRAMES)
 *					allocateFreeFrame(f, p, page, offset, numFaults);
 * -------------------------------------------
 * Allocate a free frame, prints the details
 */
void allocateFreeFrame(frameTable & f, process & p, int page, string offset)
{
	if(f.size()>=NUM_FRAMES)
	{
		cerr << "ERROR-- allocateFreeFrame: There are no free frames to allocate." << endl;
		exit(EXIT_FAILURE);
	}
	cout <<"      Using free frame" << endl;
	int frameNum = f.size();
	frame tempFrame;
	tempFrame.frameNumber = frameNum;
	tempFrame.dirty = false;
	f.push_back(tempFrame);
	p.pt[page] = frameNum;
	cout << "  Location " << frameNum << offset << endl;
}

/* Function:	isDirty
 *    Usage:	isDirty(f, frameNumber);
 * -------------------------------------------
 * Checks if the frame is dirty and needs to be paged out. Dirty bits are cleared.
 */
bool isDirty(frameTable & f, int frameNumber)
{
	for(int i=0; i<f.size(); i++)
	{
		if(f[i].frameNumber==frameNumber && f[i].dirty)
		{
			f[i].dirty = false;
			return true;
		}
	}
	return false;
}

/* Function:	setDirty
 *    Usage:	setDirty(f, frameNumber);
 * -------------------------------------------
 * Sets the frames to be dirty
 */
void setDirty(frameTable & f, int frameNumber)
{
	for(int i=0; i<f.size(); i++)
	{
		if(f[i].frameNumber==frameNumber)
		{
			f[i].dirty = true;
			break;
		}
	}
}

/* Function:	readInCommands
 *    Usage:	vector<command> c;
				readInCommands("Vm.dat", c);
 * -------------------------------------------
 * Saves the data in a formatted file (eg. "Vm.dat") into a vector of cpmmands (eg. cd).
 * Each line of the file must contain two numbers separated by a space:
 * 		- The first number is the Op Code (1-4).
 *		- The second number is the parameter (Process Size or Address).
 *		eg. "1 4605"
 */
void readInCommands(string filename, vector<command> & c)
{
	c.clear();
	ifstream f(filename, fstream::in);
	string line;
	while(f.good())
	{
		getline(f, line);
		if(line.length()>0 && isprint(line[0]))
		{
			int end = 0; //holds the current position in the line
			command newC; //holds the input
			/* skip whitespace */
			while(isblank(line[end])) end++;
			/**/
			/* if a number doesnt come next, error */
			if(!isdigit(line[end])) 
			{
				cerr << "ERROR-- readInCommands: " << filename << " '"<< line << "' - Each line MUST start with an OpCode. 1 (New Job), 2 (Read), 3 (Write), 4 (Job End)" << endl;
				exit(EXIT_FAILURE);
			}
			/**/
			/* save the Op Code if it is in the range 1-4 */
			for(; end<line.length() && isdigit(line[end]); end++) {}
			int candidate = stoi(line.substr(0,end));
			if(candidate>=1 && candidate<=4)
			{
				newC.opC = (opCode)candidate;
			}
			else
			{
				cerr << "ERROR-- readInCommands: " << filename << " '"<< line << "' - The OpCode must be one of the following: 1 (New Job), 2 (Read), 3 (Write), 4 (Job End)" << endl;
				exit(EXIT_FAILURE);
			} 
			/**/
			if(newC.opC!=4)
			{
				/* skip whitespace */
				while(isblank(line[end])) end++;
				/**/
				/* if a number doesnt come next, error */
				if(!isdigit(line[end]))
				{
					cerr << "ERROR-- readInCommands: " << filename << " '"<< line << "' - An Op Code of 1 (New Job), 2 (Read), or 3 (Write) MUST be followed by a space and a Parameter  (numbers only)" << endl;
					exit(EXIT_FAILURE);
				}
				/**/
				/* save the Parameter */
				int firstDigit = end;
				for(; end<line.length() && isdigit(line[end]); end++) {}
				newC.parameter = line.substr(firstDigit, end-firstDigit+1);
				/**/
				/* skip whitespace */
				while(isblank(line[end])) end++;
				/**/
				/* if a something comes next, error */
				if(end<line.length() && isprint(line[end]))
				{
					cerr << "ERROR-- readInCommands: " << filename << " '"<< line << "' - An Op Code of 1 (New Job), 2 (Read), or 3 (Write) MUST ONLY be followed by a space and a Parameter (numbers only)" << endl;
					exit(EXIT_FAILURE);
				}
				/**/
			}
			else
			{
				/* skip whitespace */
				while(isblank(line[end])) end++;
				/**/
				/* if a something comes next, error */
				if(end<line.length() && isprint(line[end]))
				{
					cerr << "ERROR-- readInCommands: " << filename << " '"<< line << "' - An Op Code of 4 (Job End) does not accept a parameter" << endl;
					exit(EXIT_FAILURE);
				}
				/**/
				newC.parameter="0";
			}
			c.push_back(newC);
		}
	}
	f.close();
}