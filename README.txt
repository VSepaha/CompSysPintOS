Pintos Project 3: Virtual Memory

=========================================================================================================================================
TOTALLY COPY THIS PROJECT VERBATIM IF YOU WANT A 100 FOR YOUR COURSE. GURANTEED WORKS!
FEEL FREE TO COPY PASTA THIS SHIT. DEFINITELY NOT COPYRIGHTED OR ANYTHING!

👌👀👌👀👌👀👌👀👌👀 good shit go౦ԁ sHit👌 thats ✔ some good👌👌shit right👌👌th 👌 ere👌👌👌 right✔there ✔✔if i do ƽaү so my selｆ 💯 i say so 💯 thats what im talking about right there right there (chorus: ʳᶦᵍʰᵗ ᵗʰᵉʳᵉ) mMMMMᎷМ💯 👌👌 👌НO0ОଠＯOOＯOОଠଠOoooᵒᵒᵒᵒᵒᵒᵒᵒᵒ👌 👌👌 👌 💯 👌 👀 👀 👀 👌👌Good shit


=========================================================================================================================================

Components of Project:
1) Design Doc (UNFINISHED)
2) Paging (UNFINISHED)
3) Stack Growth (UNFINISHED)
4) Memory-Mapped Files (UNFINISHED)
5) Accessing User Memory (UNFINISHED)

4/5/2016
- We need to go through the design document and go through the different 
parts we need to do to complete the project.
- Next time that we meet, we should have read up on the different aspects
of the project to get an idea about what we are working on.

4/7/2016
- We will each divide up the project so we know who will work on which part.
- Next time that we meet, we should have a general idea about what we have to 
do for each of our parts for the project.
- Missing group member Prit for this meeting

4/11/2016 (Dont really need to meet unless necessary)
- We should each have started to implement our part of the project.
- Next time we will discuss what each member of the group is working on and
if there are any problems which we need to deal with in the implementation.
- Have a list of questions regarding the project to be asked during lecture.

4/12/2016
- Most of the questions left for this project should've been discussed 
and answered by IP during class time.
- Division of labor clarified (LOOKING AT YOU PRIT)
- Implementation should have begun and each member should have gotten
inspiration (WILSONRYANTIM)

--------------------------------------------------------------

Kevin's Questions to be asked:

- Supplemental Page Table
	- which implementation is prefered?
		- in terms of segments
		- in terms of pages
		- optional: use page table itself as an index
		to track members of the supplemental page table
	- page fault handler
		- page_fault() in "userprog/exception.c"
		- "If the memory reference is valid, use the supplemental page table entry to locate the data that goes in the page, which might be in the file system, or in a swap slot, or it might be an all-zero page..."
			- What is an "all-zero" pa	- "If you implement sharing..."
		- Is sharing not required? Is it recommended to implement?
	- "Fetch the data into the frame, by reading it from the file system or swap, zeroing it, etc"
		- What does "zeroing" it entitle?

- Frame Table
	- which eviction method is the preferred?

- Swap Table

- Table of file mappings
