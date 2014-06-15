/*
  Qt-like signals using only standard C++11
  - "Slots" are just functions (static or lambdas)
  - "Signals" are "emitted", meaning they call all connected slots.

  Author: Alfred Bratterud<alfred.bratterud@hioa.no>
  
 */

#ifndef SIGNALS_HPP
#define SIGNALS_HPP

#include <vector>

namespace signals{
  
  enum emit_type{synch,asynch};
  
  template<typename F,emit_type etype>
  class signal{
    
    //Vector of functions to call on "emit"
    std::vector< std::function<F> > funcs; 
    
  public:
    //Connect a "slot" to this signal
    void connect( std::function<F> fn){
      funcs.push_back(fn);
    }
    
    //Emit this signal (i.e. call all funcs)
    template<typename... Args>
    void emit(Args... args){
      std::vector< std::future<void> > futures; 

      for(auto fn=funcs.begin(); fn!=funcs.end();fn++){
	auto sig=[&,fn](Args... args){
	  (*fn)(args...);
	};

	if(etype==asynch) 
	  futures.push_back(std::async(sig,args...));
	else 
	  sig(args...);
      }      	
    }        
  };
}

#endif
