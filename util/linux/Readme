-----------------------------------------------------------------
                    Popescu Andrei Gabriel 
                            333CA
                        Tema 2 SO
                     STD_IO Custom Lib
-----------------------------------------------------------------

-----------------------------------------------------------------
Precizare: tema a fost realizata total pe GitHub, pe un repository
privat. Am ales sa nu folsesc GitLab pentru ca nu presupunea sa 
folosesc un sistem cu totul nou.
-----------------------------------------------------------------

Structura:

.
├── constant_values.h
├── Makefile
├── Readme
├── so_stdio.c
├── so_stdio.h
└── utility.h


-----------------------
Modul de implementare
-----------------------
Am ales sa implementez pe ambele platforme Structura de fisier
relativ similara. O sa folosesc in exemplificare Structura 
temei de pe linux in randurile urmatoare:

/** FILE STRUCTURE **/
struct _so_file {
	int _file_descriptor;
	unsigned char _buffer_read[BUFFSIZE];
	unsigned char _buffer_write[BUFFSIZE];
	int _current_index_read; /* index for read buffer */
	int _current_limit_read; /* the limit of the read buffer */
	int _current_index_write; /* index for write buffer */
	int _read_flag; /* if the read buffer is used */
	int _write_flag; /* if the write buffer is used */
	int _dirty_buffer_write; /* if the last operation was read */
	int _error_flag; /* store the exiting error during process */
	int _child_process_id; /* for popen only */
};

Am folosit doua buffere tocmai pentru a separa mai bine procesul 
de read si de write. Prima varianta folosea un singur buffer si mai
putine date auxiliare in structura dar mi s-a parut mult mai "clean"
in acest mod. In principiu ambele buffere sunt pentru a limita numarul
de sys calls pe care le-ar face sistemul de fisiere pentru read si 
write. 

Trebuie precizat ca flagurile de read / write sunt doar pentru calcule 
auxilitare la ftell, se putea la fel de bine face cu un contor global
care doar asta face, aduna 1 la fiecare operatie de read si scade 1
la operatia de write de atomica (adica fgetc si fputc).

Result implementarii este sugerata de flow ul de comentarii.

----------
Referinte
----------
https://code.woboq.org/userspace/glibc/libio/stdio.h.html
https://pubs.opengroup.org/onlinepubs/009695399/functions/popen.html


---------
Probleme
---------
Un test random de la write nu merge si nu pot intelge de ce.
Nu as vrea sa cred ca este o greseala mica dar nu am avut timp
de inspecatat tot.


Overall o tema destul de intersanta.