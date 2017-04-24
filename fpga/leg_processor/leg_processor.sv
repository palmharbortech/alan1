`timescale 1 ns/10 ps

module leg_processor
  (
   input        clk,
   output [7:0] led
   );

   localparam LOAD_ABS     = 4'b0000;
   localparam ADD          = 4'b0001;
   localparam MULTIPLY     = 4'b0010;
   localparam HALT         = 4'b0011;
   localparam JUMP_ABS     = 4'b0100;

   reg [31:0][7:0] mem;
   reg [7:0]       mem_a_next;
   reg [7:0]       ip, ip_next;
   reg [3:0]       op_code;
   reg [5:0]       a;
   reg [5:0]       b;
   integer         i;

   initial begin
      for (i = 0; i < 32; i = i + 1)
        mem[i] = 8'b0;

      // Write the firmware in mem.
      // 16-bit Instruction format: oooo aaaaaa bbbbbb
      // oooo   - 4-bit opcode
      // aaaaaa - 6-bit operand A
      // bbbbbb - 6-bit operand B
      mem[17:16] = 16'b0000_000000_000001; // mem[0] <- 1
      mem[19:18] = 16'b0000_000001_000010; // mem[1] <- 2
      mem[21:20] = 16'b0001_000000_000001; // mem[0] <- mem[0] + mem[1]
      mem[23:22] = 16'b0010_000000_000001; // mem[0] <- mem[0] * mem[1]
      mem[25:24] = 16'b0100_010000_000000; // jump to instruction at byte 16
      mem[27:26] = 16'b0011_000000_000000; // halt

      ip = 8'b0001_0000; // The first instruction is at byte 16
      op_code = 4'b0;
      a = 6'b0;
      b = 6'b0;
   end

   assign led = mem[a];

   always @* begin
      mem_a_next = mem[a]; // default

      // Each instruction is 2 bytes long, so increment the instruction pointer by 2
      ip_next = ip + 2'd2;

      // decode instruction
      op_code = mem[ip + 1'b1][7:4];
      a = {mem[ip + 1'b1][3:0], mem[ip][7:6]};
      b = {mem[ip][5:0]};

      case (op_code)
        LOAD_ABS: begin
           mem_a_next = b;
        end
        ADD: begin
           mem_a_next = mem[a] + mem[b];
        end
        MULTIPLY: begin
           mem_a_next = mem[a] * mem[b];
        end
        HALT: begin
           ip_next = ip;
        end
        JUMP_ABS: begin
           ip_next = a;
        end
        default: begin
        end
      endcase
   end

   always @(posedge clk) begin
      mem[a] <= mem_a_next;
      ip <= ip_next;
   end
endmodule
