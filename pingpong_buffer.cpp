#include <systemc.h>

class write_if : virtual public sc_interface
{
    public:
        virtual void write(char) = 0;
        virtual void reset() = 0;
        virtual void write_end(bool) = 0;
};

class read_if : virtual public sc_interface
{
    public:
        virtual void read(char &) = 0;
        virtual int num_available() = 0;
        virtual bool read_end() = 0;
};

class fifo : public sc_channel, public write_if, public read_if
{
    public:
        fifo(sc_module_name name) : sc_channel(name), num_elements(0), first(0), num_elements2(0), first2(0), w_1(1), w_2(0), r_1(0), r_2(0), end_(0) {}
				
  		void write_end(bool b) {
          	if (b == true) {
            	end_ = true;
            }
        }
  
        void write(char c) {
          	// If buffer 1 is full
        	if (num_elements == max) {
                w_1 = false;
          		r_1 = true;
            }
          
          	// If buffer 2 is full
          	if (num_elements2 == max) {
                w_2 = false;
          		r_2 = true;
            }
				
          	if (w_1) {
            	data[(first + num_elements) % max] = c;
            	++ num_elements;
              	cout << sc_time_stamp() << " write buffer 1 " << c << endl;
            	//write_event.notify();
            	if (num_elements == max) {
              		w_1 = false;
                  if (num_elements2 == 0) {
                  	w_2 = true;
                  } 
                }
            } else if (w_2) {
            	data2[(first2 + num_elements2) % max] = c;
            	++ num_elements2;
              	cout << sc_time_stamp() << " write buffer 2 " << c << endl;
            	//write_event.notify();
            	if (num_elements2 == max) {
              		w_2 = false;
                  	if (num_elements == 0) {
                      w_1 = true;
                  	}
                }
            }
         
        }

        void read(char &c){
          	// If buffer 1 is empty
          	if (num_elements == 0) {
                r_1 = false;
          		w_1 = true;
            }
          
          	if (num_elements2 == 0) {
                r_2 = false;
          		w_2 = true;
            }
          
          
          	if (r_1) {
            	c = data[first];
            	-- num_elements;
            	first = (first + 1) % max;
              	cout << sc_time_stamp() << "\t\t\tread buffer 1 " << c << endl;
            //read_event.notify();
              	if (num_elements == 0) {
                	r_1 = false;
                  	w_1 = true;
                }
            } else if (r_2) {
            	c = data2[first2];
            	-- num_elements2;
            	first2 = (first2 + 1) % max;
              	cout << sc_time_stamp() << "\t\t\tread buffer 2 " << c << endl;
            //read_event.notify();
              	if (num_elements2 == 0) {
                	r_2 = false;
                  	w_2 = true;
                }
            }
          
          	
          	// If there is no new char, read the residual char. 
          	if (num_elements != 0 && end_ && r_1) {  // If buffer 1 is held and characters are remained, keep holding it.
            	r_1 = true;
            } else if (num_elements2 != 0 && end_ && r_2) {  // If buffer 2 is held and characters are remained, keep holding it. 
            	r_2 = true;
            } else if (num_elements != 0 && end_ && num_elements2 == 0) {  // If buffer 2 just finished reading, change to buffer 1.
            	r_1 = true;
            } else {
            	r_2 = true;
            }
        }

        void reset() { num_elements = first = num_elements2 = first2 = 0; }
		
        int num_available() { return num_elements;}
  		bool read_end() { if(num_elements == 0 && num_elements2 == 0 && end_) return true;
                          else return false;}		
		  
    private:
        enum e { max = 10 };
        char data[max], data2[max];
        int num_elements, first;
  		int num_elements2, first2;
  		bool w_1, w_2, r_1, r_2, end_;
};

class producer : public sc_module
{
    public:
        sc_port<write_if> out;
        sc_in<bool> clk;


        SC_HAS_PROCESS(producer);

        producer(sc_module_name name) : sc_module(name)
        {
            SC_CTHREAD(main, clk.pos());
        }

        void main()
        {
            const char *str =
                "Visit www.accellera.org and see what SystemC can do for you today!\n";

            while (*str) {
                out->write(*str++);
                wait();
            }
          	out->write_end(true);
        }
};

class consumer : public sc_module
{
    public:
        sc_port<read_if> in;
        sc_in<bool> clk;
  		
  		bool res = true;

        SC_HAS_PROCESS(consumer);

        consumer(sc_module_name name) : sc_module(name)
        {
            SC_CTHREAD(main, clk.pos());
       
        }

        void main()
        {
            char c;
            while (res) {
              	wait();
                in->read(c);
              
              	//if (in->read_end()) {
                //	res = false;
                //}
            }
        }
};

class top : public sc_module
{
    public:
        fifo *fifo_inst;
        producer *prod_inst;
        consumer *cons_inst;

        top(sc_module_name name) : sc_module(name)
        {
            fifo_inst = new fifo("Fifo1");

            prod_inst = new producer("Producer1");
            prod_inst->out(*fifo_inst);

            cons_inst = new consumer("Consumer1");
            cons_inst->in(*fifo_inst);
        }
};

int sc_main (int, char *[]) {
    top top1("Top1");
    sc_clock clk("clk", 1, SC_SEC, 1, 0, SC_SEC, true);
    
    top1.prod_inst->clk(clk);
    top1.cons_inst->clk(clk);
    sc_start(100, SC_SEC);
    return 0;
}