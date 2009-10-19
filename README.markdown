peak - parallelism exploration assembly kit
-------------------------------------------

*peak* is a collection of portable C primitives to allow fast assembly and
experimentation with task- and data-parallelism based on job pools. Different
queues and other data-exchange mechansims between different threads are offered
aside ways to group threads to mirror the platforms hardware at runtime.

*peak* target are games and other main loop based soft realtime apps.

Simplicity and making function execution context explicit are two of the prime
guiding ideas behind *peak*. Another important decision is that constructs
based on *peak* should make it possible to parallelize an application without
direct access to the platforms raw threading fascilities. Apple's 
Grand Central Dispatch, Intel's Threading Building Blocks (TBB), 
Microsoft's parallel runtime, and OpenCL are inspirations for *peak*.

*peak* values throughput and exploitation of the platforms parallel potential 
higher than special-case peak performance on a specific platform.



### Building ###

*peak* depends on [*amp*](http://github.com/bjoernknafla/amp) for low-level
cross-platform thread support. 


### Warning ###

*peak* just started to aggregate in code - and it will change, transform, and
grow before it is really usable.


### Acknowledgements ###



### Author(s) and Contact ###

You have got questions, burn to critisize me, or just want to say hello? I am 
looking forward to hear from you!

Bjoern Knafla  
Bjoern Knafla Parallelization + AI + Gamedev Consulting  
[amp@bjoernknafla.com](mailto:peak@bjoernknafla.com)  
[www.bjoernknafla.com](http://www.bjoernknafla.com)  


### Copyright and License ###

*peak* is free software. You may copy, distribute, and modify it under the terms
of the license contained in the file COPYRIGHT.txt distributed with this
package. This license is equal to the Simplified BSD License.

*amp* was developed to parallelize the AiGameDev.com Sandbox and as a 
foundation to experiment and research job pools for computer and video games.
Joint ownership of the copyright belongs to [AiGameDev.com](http://AiGameDev.com).


### Building ###



### References ###



### Disclaimer ###

All trademarks copyrights belong to their respective trademark holders and
copyright owners.


### Release Notes ###

#### Version 0.0.0 (October 19, 2009) ####

 *  Project start





