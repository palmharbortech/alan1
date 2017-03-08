`timescale 1 ns/10 ps

module ulpi_registers_testbench;
   reg test_switch;
   reg test_clk;
   reg test_ulpi_dir;
   reg test_ulpi_nxt;
   wire test_ulpi_stp;
   wire [7:0] test_led;
   wire [7:0] test_ulpi_data;

   wire [7:0] data_input;
   reg [7:0]  data_output;
   reg        data_output_valid;
   reg        stop;

   defparam uut.IN_SIMULATION = 1;
   ulpi_registers uut(
                      .switch(test_switch),
                      .clk(test_clk),
                      .ulpi_dir(test_ulpi_dir),
                      .ulpi_nxt(test_ulpi_nxt),
                      .ulpi_stp(test_ulpi_stp),
                      .led(test_led),
                      .ulpi_data(test_ulpi_data)
                      );

   assign data_input = test_ulpi_data;
   assign test_ulpi_data = (data_output_valid == 1'b1) ? data_output : 8'hZZ;

   initial begin
      test_clk = 1'b0;
      stop = 1'b0;
      while (stop != 1'b1) begin
         #5 test_clk = ~test_clk; // 100 MHz
      end
   end

   initial begin
      test_switch = 1'b1; // release switch
      test_ulpi_dir = 1'b0;
      test_ulpi_nxt = 1'b0;
      data_output = 8'b0;
      data_output_valid = 1'b0;

      // Addr 0
      #20;
      test_switch = 1'b0; // press switch
      #20;
      test_switch = 1'b1; // release switch
      @(posedge data_input[0] or
        posedge data_input[1] or
        posedge data_input[2] or
        posedge data_input[3] or
        posedge data_input[4] or
        posedge data_input[5] or
        posedge data_input[6] or
        posedge data_input[7]);
      #25;
      test_ulpi_nxt = 1'b1;
      #10;
      test_ulpi_nxt = 1'b0;
      test_ulpi_dir = 1'b1;
      #10;
      data_output = 8'b0000_1111; // Reg Data
      data_output_valid = 1'b1;
      #10;
      data_output = 8'b0;
      data_output_valid = 1'b0;
      test_ulpi_dir = 1'b0;

      // Addr 1
      #20;
      test_switch = 1'b0; // press switch
      #20;
      test_switch = 1'b1; // release switch
      @(posedge data_input[0] or
        posedge data_input[1] or
        posedge data_input[2] or
        posedge data_input[3] or
        posedge data_input[4] or
        posedge data_input[5] or
        posedge data_input[6] or
        posedge data_input[7]);
      #25;
      test_ulpi_nxt = 1'b1;
      #10;
      test_ulpi_nxt = 1'b0;
      test_ulpi_dir = 1'b1;
      #10;
      data_output = 8'b1111_0000; // Reg Data
      data_output_valid = 1'b1;
      #10;
      data_output = 8'b0;
      data_output_valid = 1'b0;
      test_ulpi_dir = 1'b0;

      // Addr 2 - longer pause
      #20;
      test_switch = 1'b0; // press switch
      #20;
      test_switch = 1'b1; // release switch
      @(posedge data_input[0] or
        posedge data_input[1] or
        posedge data_input[2] or
        posedge data_input[3] or
        posedge data_input[4] or
        posedge data_input[5] or
        posedge data_input[6] or
        posedge data_input[7]);
      #45; // longer
      test_ulpi_nxt = 1'b1;
      #10;
      test_ulpi_nxt = 1'b0;
      test_ulpi_dir = 1'b1;
      #10;
      data_output = 8'b0101_0101; // Reg Data
      data_output_valid = 1'b1;
      #10;
      data_output = 8'b0;
      data_output_valid = 1'b0;
      test_ulpi_dir = 1'b0;

      #100;
      stop = 1'b1;
   end
endmodule
