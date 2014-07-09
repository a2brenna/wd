#include "pitbull.h"
#include <hgutil/time.h>

void Task_Data::beat(){
    int s = ivals.size();
    while( s > (max_ivals - 1) ){
        ivals.pop_back();
    }

    auto current = milli_time() / 1000.0;

    if (last != -1 ){
        ivals.push_front(current - last);
    }

    last = current;

    //calculate expiration
    //
    ///update itimer for expiration wakeup

    return;
}

void Pitbull::handle(Task *t){
    if(Incoming_Connection *i = dynamic_cast<Incoming_Connection *>(t)){

    }
    else{
        throw Handler_Exception("Bad task");
    }
}
