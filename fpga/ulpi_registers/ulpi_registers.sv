/*
 * ulpi_registers.sv
 * 
 * Upon each button press, read each register from USB3320, starting at
 * address 0, and display contents via LEDs.
 */

`timescale 1 ns/10 ps

module ulpi_registers
  #(
    parameter IN_SIMULATION = 0
    )(
      input            switch,
      input            clk,
      input            ulpi_dir,
      input            ulpi_nxt,
      output           ulpi_stp,
      output reg [7:0] led,
      inout [7:0]      ulpi_data
      );

   localparam WIDTH               = 4;
   localparam IDLE                = 4'd0;
   localparam READY               = 4'd1;
   localparam SWITCH_PRESSED      = 4'd2;
   localparam SWITCH_RELEASED     = 4'd3;
   
   reg [WIDTH-1:0]  state, state_next;
   reg [7:0]        led_next;
   wire [7:0]       ulpi_data_input;
   wire             switch_debounced;
   wire             switch_pressed;
   wire             switch_released;
   reg [17:0]       delay, delay_next;
   reg [5:0]        ulpi_reg_read_addr, ulpi_reg_read_addr_next;
   reg              ulpi_reg_read_en, ulpi_reg_read_en_next;
   wire             ulpi_reg_read_data_ready;
   wire [7:0]       ulpi_reg_read_data;
   reg [5:0]        ulpi_reg_write_addr, ulpi_reg_write_addr_next;
   reg [7:0]        ulpi_reg_write_data, ulpi_reg_write_data_next;
   reg              ulpi_reg_write_en, ulpi_reg_write_en_next;
   
   debouncer debouncer0(clk, switch, switch_debounced);
   ulpi_reg_read ulpi_reg_read0(clk,
                                ulpi_reg_read_addr,
                                ulpi_reg_read_en,
                                ulpi_dir,
                                ulpi_nxt,
                                ulpi_data,
                                ulpi_stp,
                                ulpi_reg_read_data,
                                ulpi_reg_read_data_ready
                                );

   initial begin
      led = 8'b0;
      state = 1'b0;
      delay = 18'b0;
      ulpi_reg_read_addr = 6'b0;
      ulpi_reg_read_en = 1'b0;
      ulpi_reg_write_addr = 1'b0;
      ulpi_reg_write_data = 1'b0;
      ulpi_reg_write_en = 1'b0;
   end

   // Combinational logic using assign
   assign switch_pressed = IN_SIMULATION ? ~switch : ~switch_debounced;
   assign switch_released = IN_SIMULATION ? switch : switch_debounced;

   // Combinational logic using always
   always @* begin
      // defaults
      state_next = state;
      delay_next = delay;
      led_next = led;
      ulpi_reg_read_addr_next = ulpi_reg_read_addr;
      ulpi_reg_read_en_next = ulpi_reg_read_en;
      ulpi_reg_write_addr_next = ulpi_reg_write_addr;
      ulpi_reg_write_data_next = ulpi_reg_write_data;
      ulpi_reg_write_en_next = ulpi_reg_write_en;

      case (state)
        IDLE: begin
           // about 4ms for 60MHz clk
           if ((IN_SIMULATION == 0 && delay == {18{1'b1}}) ||
               (IN_SIMULATION == 1 && delay == 18'b1)) begin
              state_next = READY;
           end else begin
              delay_next = delay + 1'b1;
           end
        end
        READY: begin
           if (switch_pressed)
             state_next = SWITCH_PRESSED;
        end
        SWITCH_PRESSED: begin
           if (switch_released)
             state_next = SWITCH_RELEASED;
        end
        SWITCH_RELEASED: begin
           ulpi_reg_read_en_next = 1'b1;
           if (ulpi_reg_read_data_ready == 1'b1) begin
              led_next = ulpi_reg_read_data;
              ulpi_reg_read_addr_next = ulpi_reg_read_addr + 1'b1;
              ulpi_reg_read_en_next = 1'b0;
              state_next = READY;
           end
        end
        default: state_next = IDLE;
      endcase
   end

   // Sequential logic
   always @(posedge clk) begin
      state <= state_next;
      delay <= delay_next;
      led <= led_next;
      ulpi_reg_read_addr <= ulpi_reg_read_addr_next;
      ulpi_reg_read_en <= ulpi_reg_read_en_next;
      ulpi_reg_write_addr <= ulpi_reg_write_addr_next;
      ulpi_reg_write_data <= ulpi_reg_write_data_next;
      ulpi_reg_write_en <= ulpi_reg_write_en_next;
   end
endmodule
