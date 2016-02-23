Alarm Clock:
    The alarm clock is complete and the files pertaining to it (listed below) are provided.
    1) timer.c
    2) thread.c
    3) thread.h
    In general, to eliminate busy waiting we use a list. Each time a timer_sleep is called, a thread is added to a list and blocked.
    Then during each interrupt we check to see if it is time for a thread to wake up. If the thread it ready to be woken up then we 
    remove it from the list and unblock it.

Priority Scheduling:
    Files TBD
