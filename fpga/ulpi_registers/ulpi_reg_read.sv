`timescale 1 ns/10 ps

module ulpi_reg_read
  (
   input            clk,
   input [5:0]      addr,
   input            en,
   input            ulpi_dir,
   input            ulpi_nxt,
   inout [7:0]      ulpi_data,
   output reg       ulpi_stp,
   output reg [7:0] data,
   output reg       data_ready
   );

   localparam WIDTH               = 4;
   localparam IDLE                = 4'd0;
   localparam WAIT_FOR_NXT        = 4'd1;
   localparam WAIT_FOR_DIR        = 4'd2;
   localparam READ_REG_DATA       = 4'd3;
   localparam WAIT_FOR_DIR_LOW    = 4'd4;

   reg [WIDTH-1:0]  state, state_next;
   wire [7:0]       ulpi_data_input;
   reg [7:0]        ulpi_data_output, ulpi_data_output_next;
   reg              ulpi_data_output_valid, ulpi_data_output_valid_next;
   reg [7:0]        data_next;
   reg              data_ready_next;
   reg              ulpi_stp_next;
   reg              has_written, has_written_next;

   initial begin
      data = 8'b0000_0000;
      data_ready = 1'b0;
      state = 1'b0;
      ulpi_data_output = 1'b0;
      ulpi_data_output_valid = 1'b1;
      ulpi_stp = 1'b0;
      has_written = 1'b0;
   end

   // Combinational logic using assign
   assign ulpi_data_input = ulpi_data;
   assign ulpi_data = (ulpi_data_output_valid == 1'b1) ? ulpi_data_output : 8'hZZ;

   // Combinational logic using always
   always @* begin
      // defaults
      state_next = state;
      data_ready_next = data_ready;
      ulpi_data_output_next = ulpi_data_output;
      ulpi_data_output_valid_next = ulpi_data_output_valid;
      data_next = data;
      ulpi_stp_next = ulpi_stp;
      has_written_next = has_written;

      case (state)
        IDLE: begin
           if (en && ~ulpi_dir) begin
              ulpi_data_output_next = {2'b11, addr}; // Register Read
              state_next = WAIT_FOR_NXT;
           end
        end
        WAIT_FOR_NXT: begin
           if (ulpi_nxt) begin
              state_next = WAIT_FOR_DIR;
           end
        end
        WAIT_FOR_DIR: begin
           if (ulpi_dir) begin
              ulpi_data_output_next = 8'b0;
              ulpi_data_output_valid_next = 1'b0;
              state_next = READ_REG_DATA;
           end
        end
        READ_REG_DATA: begin
           data_next = ulpi_data_input;
           data_ready_next = 1'b1;
           state_next = WAIT_FOR_DIR_LOW;
        end
        WAIT_FOR_DIR_LOW: begin
           if (!ulpi_dir) begin
              ulpi_data_output_valid_next = 1'b1;
              data_ready_next = 1'b0;
              state_next = IDLE;
           end
        end
        default: state_next = IDLE;
      endcase
   end

   // Sequential logic
   always @(posedge clk) begin
      state <= state_next;
      ulpi_data_output <= ulpi_data_output_next;
      ulpi_data_output_valid <= ulpi_data_output_valid_next;
      data <= data_next;
      data_ready <= data_ready_next;
      ulpi_stp <= ulpi_stp_next;
      has_written <= has_written_next;
   end
endmodule
