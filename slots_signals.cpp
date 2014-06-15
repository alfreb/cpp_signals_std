/*
  A little demo using signals.hpp
  - A "vm" class emulates a virtual machine
    - It "signals" when booted and when it has new data
    - It subscribes to "broadcast" signal from server
  - A "server" class emulates a host to run vm's
    - It subscribes to "booted" signal from vm's
    - It signals vm's with tasks, using the "broadcast" signal

  Author: Alfred Bratterud<alfred.bratterud@hioa.no>

*/
#include <future>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

//Debug
#include <assert.h>

//Local
#include "signals.hpp"

using namespace std;

class vm{
  int id; 
  bool booted=false;
  int boot_count=0;
  random_device rand;
  chrono::milliseconds ms{2000};

  //Receive "tasks" from a server
  vector<string> tasks{};  

  //Private signal, when VM has received data
  signals::signal<void(string), signals::synch> signal_data; 
  
public:
  
  //Public signal boot (Caveat: anyone can emit)
  signals::signal<void(int), signals::synch> signal_boot;

  //Construct / copy-construct
  vm(int i):id{i}{ cout << "Created vm " << i << endl;};  
  vm(const vm& v)
    :id{v.id},booted{v.booted},boot_count{v.boot_count}
  {};

  int get_id(){return id;}

  void boot(){
    //Make sure we don't get called twice 
    assert(!booted); //Assertion 1: Just for debugging 
    
    //Emulate work, such as starting a vm in a subprocess
    this_thread::sleep_for(ms);

    //Emit the boot signal
    signal_boot.emit(id); 
    booted=true;    
    
    //Set a random activity interval
    chrono::milliseconds ms{500+rand()%7000};
    while(booted){
      //Emulate some work, generate data, "do" tasks from host
      this_thread::sleep_for(ms);
      string data=string("<html>... VM ")+to_string(id)+string("...</html>");	
      if(tasks.size()>0){
	string task=tasks.back();
	tasks.pop_back();
	data=string("VM ")+to_string(id)+" Solving task: "+task;	
      }
      signal_data.emit(data); 
    }
  }

  //Expose connector to a private signal
  void on_data_out(function< void(string)> fn){
    signal_data.connect(fn);
  }
  
  //"Slot" for server broadcast. Should be connected to a synchronous signal
  void data_in(string s){
    tasks.push_back(s);
    //Emulate some more iowait, before signalling that we have data
    this_thread::sleep_for(ms);    
    
    //Emit the data signal
    signal_data.emit(string(to_string(id)+": Task recieved: ")+s);
    
  }
  
  void kill(){
    cout << "Vm " << id << " going down." << endl;
    booted=false;
  }
  
};


class server{
  int booted{0};
  
  //Some synchronization needed
  mutex mtx;
  
  //Add some vms
  vector<vm> vms{1,2,3,4,5,6,7,8,9,10};

  //A signal for giving "tasks" to all vms.
  signals::signal< void(string), signals::asynch > signal_broadcast;  

  //"Slot" for notification on VM-boot - self-synchronized
  void vm_booted(int i){
    mtx.lock();
    booted++;
    cout << "Vm "<< i << " booted from thread  "
	 << this_thread::get_id()<< ". " 
	 << booted << " vm's are up" << endl;
    mtx.unlock();
  }
  
  //"Slot" for receiving data from VM's - self-synchronized
  void vm_data(string s){
    mtx.lock();
    cout << "Data from VM: " << s << endl;
    mtx.unlock();
  }

  
public:
  //Boot all vms, supply some tasks, call it a day.
  void run(){
    vector< future<void> > futures;
    
    for(auto v=vms.begin(); v!=vms.end(); v++){    
      //Connect to the vm-signals
      v->signal_boot.connect([&](int i){
	  vm_booted(i);
	});
      
      v->on_data_out([&](string s){
	  vm_data(s);
	});  
      
      //Connect vms to the "broadcast signal"
      //Must remmeber to pass in the iterator by value
      //or Assertion 1 (above) will fail
      signal_broadcast.connect([v](string s){
	  v->data_in(s);
	});
      
      cout << "Initiating boot on VM " << v->get_id() << endl;
      
      //We also save the future, so it doesn't go out of scope.
      futures.push_back(async([v](){
	    v->boot();
	  }));
      
    }
    
    //Command the minions / give them a break
    signal_broadcast.emit("Do maintenence");
    
    this_thread::sleep_for(chrono::seconds{20}); 
    
    signal_broadcast.emit("Take backup");   

    this_thread::sleep_for(chrono::seconds{30});

    //Ok, let's call it a day
    for(auto v=vms.begin();v!=vms.end();v++)
      v->kill();   
  };  
};


int main(){    
  server host;
  host.run();  
}
