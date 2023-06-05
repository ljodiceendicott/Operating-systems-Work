//============================================================================
// Name        : OS-Scheduler.cpp
// Author      : M. Ocean
// Version     : 2.3
// Copyright   : Free to use
// Description : Program to simulate various scheduling policies
//					covered in class.
//============================================================================

#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdlib.h>
#include <string.h>

using namespace std;

/**********************************************************
 * GLOBALS
 *********************************************************/
#define MAX_JOBS 20

enum scheduler_t {
	FCFS=0, SJF=1, HRRN=2, PSJF=3, RRq1=4, RRq4=5
};

scheduler_t scheduler_num; // the scheduling policy we are currently using 
struct job_t
{
	int pid;			// process id
	int arrival;		// arrival time
	int service_total;	    // total service time requested
	int service_remaining;	// service time remaining
	int started_at;		// time job started
	int finished_at;	// time when job finished

	int turn_around;		// turn_around time (Time_start - Time_arrival)
						//   NOTE: value is set when the job is started
	int waiting;		// total time spent waiting (total time not running)
						//   NOTE: value is set when the job is finished
	float normalized_turn_around; // TODO: compute this at the end of a job.
							//  notice this is a float, remember C/C++ division rules
};

job_t jobs[MAX_JOBS];   // an array of all jobs in the system
int num_jobs;

int current_time;				// the time "in ticks"
int time_of_last_job_change=0;	// time we last picked a new job
int time_quantum = 0;			// the value of q for RR (ignored otherwise)

int rr_queue[MAX_JOBS];	// an array that you can use as a queue for round-robin
int rr_length;			// number of jobs in the rr_queue

int sched_log[100];		// an array of the schedule for GANNT chart.


/**********************************************************
 * Function PROTOTYPES / DECLARATIONS
 *********************************************************/
int loadjobs(char[]);	// loads jobs from a file
void showjobs();		// prints status (for debugging)
void runJobForOne(int);	// runs a job for 1 time unit

bool reconsiderCurrentJob(int &);	// returns true if we should consider a new job
						// TODO: modify this to support preemptive jobs

void pickNextJob(int&); // calls the appropriate pickNextJob function
						// based on the scheduler selected.  [DO NOT CHANGE!]
						// NOTE: This is *only* called when reconsiderCurrentJob()
                        // has returned true

void updateNormalizedTurnAroundTime(int);

void print_gaant_as_html(int, char[]);

void pickNextJob_FCFS(int&);
void pickNextJob_HRRN(int&);
void pickNextJob_SJF(int&);
void pickNextJob_PSJF(int&);
void pickNextJob_RR(int&);

/**********************************************************
 * main()
 *********************************************************/
int main(int argc, char *argv[])
{
    int current_job = -1;
	if(argc < 2)
	{
		cout << "usage:  my_sched JOBFILE [-html HTMLFILE]" << endl
			 << "   Simulates scheduling the jobs described in jobfile." << endl << endl
		     << "Mandatory arguments:" << endl
			 << "JOBFILE: a plain text file that contains:" << endl
			 << "\t\tSCHEDULER_NUM" << endl
			 << "\t\tPID	ARR	REQ" << endl
			 << "\t\tPID	ARR	REQ" << endl
			 << "\t\tPID	ARR	REQ" << endl
			 << "\t\tPID	ARR	REQ" << endl
			 << "\t\t[...etc]" << endl
			 << endl << "SCHEDULER_NUM: FCFS=0, SJF=1, HRRN=2, PSJF=3, RRq1=4, RRq4=5" << endl
			 << "PID: \tunique integer process identifier" << endl
			 << "ARR: \tarrival time, integer (time starts at 0)" << endl
			 << "REQ: \tservice time required, integer" << endl
			 << endl << "Optional arguments:" << endl
			 << "-html HTMLFILE \t\t generate a gaant chart in html in the file HTMLFILE"
			 << endl; 
		return 0;
	}
	int totaltime = loadjobs(argv[1]);
	cout << "Total time required:" << totaltime << endl << endl;

	for(current_time=0; current_time < totaltime; current_time++)
	{
		showjobs();
		if(reconsiderCurrentJob(current_job)){
            cout << "*******************" << endl;
            cout << "@ t=" << current_time << " reconsiderCurrentJob() returned true" << endl;
		    pickNextJob(current_job);
            cout << "@ t=" << current_time << " pickNextJob() returned " << current_job << endl;
            cout << "*******************" << endl;
        }
        runJobForOne(current_job);
	}
	for(int j=0;j<num_jobs;j++){
		updateNormalizedTurnAroundTime(j);
	}
	showjobs();
	
	// let's print out the timeline, now that we are done:
	cout << endl;
	for(int i=0;i<totaltime;i++){
		cout << i << ":";
		cout << sched_log[i] << " ";
	}
	cout << endl;


	if(argc==4){
		// write html to the filename as html if requested
		print_gaant_as_html(totaltime, argv[3]);
	}
	else{
		// to standard out it goes
		print_gaant_as_html(totaltime, strdup(""));
	}
}

/**********************************************************
 * Function DEFINITIONS
 *********************************************************/
// updates the state of the current_job to reflect it running for "1"
// REQUIRES: current_job is not -1 and has time service_remaining (program will abort if not)
// MODIFIES: state of jobs[current_job] to reflect being scheduled to run for 1
void runJobForOne(int current_job)
{
	if(current_job==-1)
	{
		cout << "called runJobForOne, but current_job == -1" << endl
				<< "...something seems wrong with your pickJob.  Fix that." << endl;
		exit(-1);
	}
    if(jobs[current_job].arrival > current_time)
    {
        cout << "called runJobForOne, the job you picked hasn't arrived yet" << endl
                << "...something seems wrong with your pickJob.  Fix that." << endl;
        exit(-1);
    }
	if(jobs[current_job].service_remaining <= 0)
	{
		cout << "Called runJobForOne, but current_job has no work left" << endl
				<< "...something seems wrong with your pickJob.  Fix that." << endl;
		exit(-1);
	}

	// can compute start time now
    if(jobs[current_job].service_remaining == jobs[current_job].service_total){
        jobs[current_job].started_at = current_time;
    }

	cout << "[time=" << current_time << "] running Job pid=" << jobs[current_job].pid << endl;
	jobs[current_job].service_remaining--;


	sched_log[current_time] = jobs[current_job].pid;

	// if we just finished this job, compute the proper waiting time, turn around, and finish
	if(jobs[current_job].service_remaining == 0)
	{
		jobs[current_job].finished_at = current_time + 1;
		jobs[current_job].turn_around = jobs[current_job].finished_at - jobs[current_job].arrival;
		jobs[current_job].waiting = jobs[current_job].turn_around - jobs[current_job].service_total;
	}
}

// REQUIRES: several lines of input from file on command line (arg1)
//
// first line of input is the scheduler number [0..5] (see pickNextJob for mapping)
// next, we need 'num_jobs'-many jobs from using the format:
// pid 	arrivaltime	service_totaltime
// (all should be ints)
//
// MODIFIES: jobs[]
//
// EFFECTS: returns the total time to complete all jobs
int loadjobs(char filename[])
{
	rr_length = 0;
	int totaltime = 0;
	fstream file_in;
	file_in.open(filename,ios::in);

    int tmp;
	file_in >> tmp;
    scheduler_num = (scheduler_t)tmp;

	cout << "Scheduler selected: " << (int) scheduler_num << endl;
	if(scheduler_num==RRq1){
		time_quantum=1;
	}
	else if(scheduler_num==RRq4){
		time_quantum=4;
	}

	num_jobs = 0;
	int last_pid = 0;
	while( !file_in.eof() ){
		int this_pid;
		job_t j;
		file_in >> this_pid;
		if(this_pid == last_pid) // read error; break out now
			break;
		j.pid = this_pid;
		file_in >> j.arrival;
		file_in >> j.service_total;
		j.service_remaining = j.service_total;
		j.waiting = -1;
		j.started_at = -1;
		j.finished_at = -1;
		j.turn_around = -1;
		j.normalized_turn_around = -1;

		totaltime += j.service_total;

		jobs[num_jobs] = j;
		num_jobs++;
		last_pid = this_pid;
	} 
	//incase the times are in weird order, reorders them based on their PID
	for (int i = 0; i < num_jobs; i++)
    {
        for (int j = i + 1; j < num_jobs; j++)
        {
            if (jobs[i].pid > jobs[j].pid)
            {
                job_t job = jobs[i];
                jobs[i] = jobs[j];
                jobs[j] = job;
            }
        }
    }
	file_in.close();
	return totaltime;
}


// REQUIRES: current_time should be set to a value [0..MAXINT]
//			 jobs available in jobs[] array
//
// MODIFIES: screen: prints out time and table of jobs
void showjobs()
{
	cout << "Job status as of time [" << current_time << "]" << endl;
	cout << setw(9) << "PID  |" 
		 << setw(11) << "arr  |"
		 << setw(11) << "svc   |" 
		 << setw(8) << " svc_left |" 
		 << setw(8) << "  started |" 
		 << setw(8) << " finished |" 
		 << setw(10) << " turn_around |" 
		 << setw(10) << " normalized |" 
		 << endl;

	cout << "-------------------------------------------" << endl;
	for (int i=0; i<num_jobs; i++)
	{
		cout << setw(6) << jobs[i].pid << 
		"  |" << setw(8) << jobs[i].arrival  << 
		"  |" << setw(8) << jobs[i].service_total  << 
		"  |" << setw(8) << jobs[i].service_remaining  << 
		"  |" << setw(8) << jobs[i].started_at << 
		"  |" << setw(8) << jobs[i].finished_at << 
		"  |" << setw(10) << jobs[i].turn_around  << 
		"  |" << setw(10) << fixed << setprecision(5) << jobs[i].normalized_turn_around  << 
		"  |" << endl;
	}
	// round robin; print the RR "queue"
	if(scheduler_num==RRq1 || scheduler_num==RRq4){
		cout << "-------------------------------------------" << endl;
		cout << "  RR_queue: [";
		for(int i=0; i<rr_length; i++){
			cout << " " << rr_queue[i] << ((i==rr_length-1) ? "" : ",");
		}
		cout << "]" << endl;
	}
	cout << "===========================================" << endl;
}

// REQUIRES: scheduler to be set to a value in [0,5]
// MODIFIES: current_job (by reference) by calling one of the
//			more specific pickNextJob_ functions
void pickNextJob(int &current_job)
{
	if(scheduler_num==FCFS)
		pickNextJob_FCFS(current_job);
	else if(scheduler_num==SJF)
		pickNextJob_SJF(current_job);
	else if(scheduler_num==HRRN)
		pickNextJob_HRRN(current_job);
	else if(scheduler_num==PSJF)
		pickNextJob_PSJF(current_job);
	else if(scheduler_num==RRq1 || scheduler_num==RRq4)
		pickNextJob_RR(current_job);
	else
	{
		cout << "valid scheduler options are:" << endl
				<< "0 \t\t FCFS" << endl
				<< "1 \t\t SJF" << endl
				<< "2 \t\t HRRN" << endl
				<< "3 \t\t PSJF" << endl
				<< "4 \t\t RRq=1" << endl
				<< "5 \t\t RRq=4" << endl;
		exit(-1);
	}
	time_of_last_job_change = current_time;
}


/**********************************************************
 * Utility functions that NEED CHANGES
 *********************************************************/
// function is called every tick to see if a new job should
//    be CONSIDERED; works perfectly but only for non-preemptive
// REQUIRES: the current_job
// MODIFIES: current_job to be -1 if the current job is done.
// EFFECTS: RETURNS: 
//			true if the scheduling policy indicates a
// 					 job change should be CONSIDERED, 
//			false if the policy would not consider change now
//  hint1: current_time is global and tells us what time it is)
//  hint2: this runs every time tick, so if you need to do something
//    whenever as new job arrives for certain schedulers THIS is the 
//    place to do it...
bool reconsiderCurrentJob(int &current_job)
{
    // if we don't have a job, it's time to schedule no matter what
	if(current_job==-1){
		// (this is how the first job gets picked)
		return true;
	}

	// if current job is finished; pick a new job
	if(jobs[current_job].service_remaining==0){
        current_job=-1;
        return true;
    }
	
	// TODO: modify this as necessary.  Use information
	// about the scheduluer type, job_queue, and current_time 
	// to return true if you should be considering a new job
	// do NOT call pick_next() here; just return true/false
	if(scheduler_num == PSJF)
	{
		for(int i = 0; i< num_jobs; i++){
			if(jobs[i].arrival == current_time){
				if(jobs[i].service_total < jobs[current_job].service_remaining){
					return true;
				}
			}
		}
		return false;
	}
	if((scheduler_num == RRq1 || scheduler_num == RRq4) && (current_time -time_of_last_job_change == time_quantum)){
		return true;
	}
	
    return false;
}
// function is called at the END of all jobs running to update
//	each job's normalized turn around time.
// EFFECTS: jobs[job_num] to have the floating point value 
//	of normalized turn around time (don't forget C++ int division rules)
void updateNormalizedTurnAroundTime(int job_num){
	// TODO: implement this.
	jobs[job_num].normalized_turn_around = -1; // -1 means not computed.

	jobs[job_num].normalized_turn_around = (float)jobs[job_num].turn_around/jobs[job_num].service_total;
}

/**********************************************************
 * Scheduler policy functions that NEED CHANGES
 *********************************************************/
// REQUIRES: jobs available in jobs[] array
//
// MODIFIES: current_job to be set to the next job to run via the
//				scheduling algorithm indicated (by the function name)
void pickNextJob_FCFS(int &current_job)
{
	current_job = -1;

	for(int i=0; i < num_jobs ; i++)
	{
		// find the first job that has remaining time
		if( jobs[i].arrival <= current_time &&
			jobs[i].service_remaining > 0)
		{
			if (current_job == -1)
				current_job = i;
			else if(jobs[i].arrival < jobs[current_job].arrival)
				current_job = i;
			else if((jobs[i].arrival == jobs[current_job].arrival)
				    && jobs[i].pid < jobs[current_job].pid)
				current_job = i;
		}
	}

	if(current_job==-1){
		cout << "critical error; no job selected by scheduler!" << endl;
		exit(-1);
	}
}

// REQUIRES: jobs available in jobs[] array
// MODIFIES: current_job to be set to the next job to run via the
//	Highest Response Ration Next scheduling policy
void pickNextJob_HRRN(int &current_job)
{
	// TODO: compute HRRN to set current_job accordingly
	current_job = -1;
	float currentrr, maxrr = -1;
	for(int i = 0; i< num_jobs; i++){
		currentrr =float(((current_time - jobs[i].arrival)+jobs[i].service_total))/jobs[i].service_total;
		if( jobs[i].arrival <= current_time &&
			jobs[i].service_remaining > 0)
		{
			if(current_job == -1){
				current_job = i;
				maxrr = currentrr;
			}
			else if(currentrr > maxrr){
				maxrr = currentrr;
				current_job = i;
			} 
		}
	}
	if(current_job==-1){
		cout << "critical error; no job selected by scheduler!" << endl;
		exit(-1);
	}
}

// REQUIRES: jobs available in jobs[] array
// MODIFIES: current_job to be set to the next job to run via the
// SHORTEST JOB FIRST scheduling policy
void pickNextJob_SJF(int &current_job)
{
		current_job = -1;

	for(int i=0; i < num_jobs ; i++)
	{
		// find the first job that has remaining time
		if( jobs[i].arrival <= current_time &&
			jobs[i].service_remaining > 0)
		{
			if(current_job == -1){
				current_job = i;
			}
			if(jobs[i].service_total<jobs[current_job].service_total)
			{
				current_job = i;
			}
		}
	}

	if(current_job==-1){
		cout << "critical error; no job selected by scheduler!" << endl;
		exit(-1);
	}
	// TODO: compute shortest job and set current_job accordingly
}

// REQUIRES: jobs available in jobs[] array
// MODIFIES: current_job to be set to the next job to run via the
// STCF = Shortest Time to Completion First - AKA PREEMPTIVE SHORTEST JOB FIRST
void pickNextJob_PSJF(int &current_job)
{
			current_job = -1;

	for(int i=0; i < num_jobs ; i++)
	{
		if( jobs[i].arrival <= current_time &&
			jobs[i].service_remaining > 0)
		{
			if(current_job == -1){
				current_job = i;
			}
			if(jobs[i].service_remaining<jobs[current_job].service_remaining){
				current_job = i;
			}
		}
	}
	// TODO: write this...
}

// REQUIRES: time_quantum (global) has been set
//			 jobs available in jobs[] array
// MODIFIES: current_job to be set to the next job to run via the
//				Round Robin Scheduling algorithm
void pickNextJob_RR(int &current_job)
{
	bool found;
	
	for(int i=0; i < num_jobs ; i++)
	{
		if( jobs[i].arrival <= current_time &&
			jobs[i].service_remaining > 0 && 
			current_job != i)
		{

			found = false;
			for(int j = 0; j< rr_length; j++){
				if(rr_queue[j]==i){
					found = true;
					break;
				}
			}
			if(!found)
			{
				rr_queue[rr_length]=i;
				rr_length++;
			}
		}
		found = false;
	}
	// use RR w/ time_quantum (global) and set current_job accordingly
	// TODO: write this...
    // HINT: I give you an empty integer array rr_queue and rr_length
	// that you can use as a fake queue, put job indices in it
	 if (current_job != -1)
		{
                rr_queue[rr_length] = current_job;
				rr_length++;
    	}
	if (rr_length > 0) 
	{
		current_job = rr_queue[0];
		for (int i = 1; i < rr_length; i++) 
		{
			rr_queue[i-1] = rr_queue[i];
		}
		rr_length--;
	}
	if(current_job==-1){
		cout << "critical error; no job selected by scheduler!" << endl;
		exit(-1);
	}

}

// Print out the GANNT CHART in color coded HTML that can
// be copied and pasted into an HTML file and viewed
void print_gaant_as_html(int totaltime, char filename[])
{
	// conditionally write to a file or stdout courtesy of:
	// https://stackoverflow.com/a/366969
	ofstream file_out;
	streambuf* buff_out;

	if(strcmp(filename,"")!=0){  // we have a file
		file_out.open(filename,ios::out);
		buff_out = file_out.rdbuf();
		cout << "writing html file: '"<< filename << "'" << endl;
	}
	else{
		buff_out = cout.rdbuf();
	}
	ostream dest_out(buff_out);
	// end stack overflow code

    dest_out << "<HTML>" << endl;
	dest_out << "<H1>Chart:</H1>" << endl;
	dest_out << "<table border=\"1\">" << endl;
	// TODO: produce a proper HTML table that looks/ like a gannt chart.  
	// That means each PID has it's own TR with TD for each time, and
	// colors set for times this PID is running. this is totally not that.
	// it's just some example code to get you started
	// make sure you write to dest_out to write to either the screen or the file
	
	
	dest_out <<"<tr>"<<endl;
	dest_out <<"<th>PID</th>"<<endl;
	for(int i = 0; i< totaltime; i++){
		dest_out<<"<th>"<<i<<"</th>"<<endl;
	}
	dest_out<<"</tr>"<<endl;
	for(int j = 0; j<num_jobs; j++){ 
		dest_out<<"<tr>"<<endl;
		dest_out<<"<td>"<<jobs[j].pid<<"</td>"<<endl;
		for(int i = 0; i< totaltime; i++){
			dest_out<<"<td";
			if(sched_log[i]==jobs[j].pid){
				dest_out<<" bgcolor=rgb(0,255,207)";
			}
			dest_out<<" width=20ems>&nbsp;"<<endl;
			dest_out<<"</td>"<<endl;
		}
		dest_out<<"</tr>"<<endl;
	}
    dest_out << "</table>" << endl;
	dest_out << "<p/><p/>" << endl;
    dest_out << "<H1>Details:</H1>" << endl;
    dest_out << "<table border=\"1\">" << endl;
	dest_out <<"<tr>"<<endl;
	dest_out <<"<th>PID</th>"<<endl;
	dest_out <<"<th>Response Ratio</th>"<<endl<<"</tr>"<<endl;
    for (int i=0; i<num_jobs; i++){
        // TODO: print out each PID and it's Response Ratio
		dest_out<<"<tr>"<<endl;
		dest_out<<"<td>"<<jobs[i].pid<<"</td>"<<endl;
		dest_out<<"<td>"<<jobs[i].normalized_turn_around<<"</td>"<<endl<<"</tr>"<<endl;
    }
    dest_out << "</table>" << endl;
    dest_out << "</HTML>" << endl;
	dest_out << endl;
}
