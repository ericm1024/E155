/**
 * \file aes.sv
 * \author Eric Mueller
 * Addapted from aes_starter.sv by bchasnov@hmc.edu, David_Harris@hmc.edu
 */


/* Test AES with cases from FIPS-197 appendix */
module testbench();
   logic clk, load, done, sck, sdi, sdo;
   logic [127:0] key, plaintext, cyphertext, expected;
   logic [255:0] comb;
   logic [8:0]   i;
    
   // device under test
   aes dut(clk, sck, sdi, sdo, load, done);
    
   // test case
   initial begin   
      // Test case from FIPS-197 Appendix A.1, B
      key       <= 128'h2B7E151628AED2A6ABF7158809CF4F3C;
      plaintext <= 128'h3243F6A8885A308D313198A2E0370734;
      expected  <= 128'h3925841D02DC09FBDC118597196A0B32;
      
// Alternate test case from Appendix C.1
//      key       <= 128'h000102030405060708090A0B0C0D0E0F;
//      plaintext <= 128'h00112233445566778899AABBCCDDEEFF;
//      expected  <= 128'h69C4E0D86A7B0430D8CDB78070B4C55A;
   end
    
   // generate clock and load signals
   initial 
     forever begin
        clk = 1'b0; #5;
        clk = 1'b1; #5;
     end
   
   initial begin
      i = 0;
      load = 1'b1;
   end 
   
   assign comb = {plaintext, key};
   // shift in test vectors, wait until done, and shift out result
   always @(posedge clk) begin
      if (i == 256) load = 1'b0;
      if (i<256) begin
         #1; sdi = comb[255-i];
         #1; sck = 1; #5; sck = 0;
         i = i + 1;
      end else if (done && i < 384) begin
         #1; sck = 1; 
         #1; cyphertext[383-i] = sdo;
         #4; sck = 0;
         i = i + 1;
      end else if (i == 384) begin
         if (cyphertext == expected)
           $display("Testbench ran successfully");
         else $display("Error: cyphertext = %h, expected %h",
                       cyphertext, expected);
         $stop();
         
      end
   end   
endmodule


/** 
 * Top level module with SPI interface and SPI core
 * 
 * \param clk    40MHz board clock
 * \param sck    serial clock
 * \param sdi    slave data in, aka MOSI
 * \param sdo    slave data out, aka MISO
 * \param load   load signal from Pi's GPIO pin
 * \param done   done signal to Pi's GPIO pin
 */
module aes(input  logic clk,
           input  logic sck, 
           input  logic sdi,
           output logic sdo,
           input  logic load,
           output logic done);
                    
   logic [127:0]        key, plaintext, cyphertext;
   
   aes_spi spi(sck, sdi, sdo, done, key, plaintext, cyphertext);   
   aes_core core(clk, load, key, plaintext, done, cyphertext);
endmodule


/**
 * SPI interface.  Shifts in key and plaintext Captures ciphertext when done,
 * then shifts it out.
 * 
 * \param sck    serial clock
 * \param sdi    slave data in, aka MOSI
 * \param sdo    slave data out, aka MISO
 * \param done   done signal to Pi's GPIO pin
 * \param key    key read from Pi
 * \param plaintext     plaintext read from Pi
 * \param cyphertext    cyphertext to write back to the Pi
 */
module aes_spi(input  logic sck, 
               input  logic sdi,
               output logic sdo,
               input  logic done,
               output logic [127:0] key, plaintext,
               input  logic [127:0] cyphertext);

   logic                            sdodelayed, wasdone;
   logic [127:0]                    cyphertextcaptured;


   /* Tricky cases to properly change sdo on negedge clk. */

   // assert load
   // apply 256 sclks to shift in key and plaintext, starting with plaintext[0]
   // then deassert load, wait until done
   // then apply 128 sclks to shift out cyphertext, starting with cyphertext[0]
   always_ff @(posedge sck)
     if (!wasdone)  {cyphertextcaptured, plaintext, key} = {cyphertext, plaintext[126:0], key, sdi};
     else           {cyphertextcaptured, plaintext, key} = {cyphertextcaptured[126:0], plaintext, key, sdi}; 
   
   // sdo should change on the negative edge of sck
   always_ff @(negedge sck) begin
      wasdone = done;
      sdodelayed = cyphertextcaptured[126];
   end
    
   // when done is first asserted, shift out msb before clock edge
   assign sdo = (done & !wasdone) ? cyphertext[127] : sdodelayed;
endmodule


/**
 * top level AES encryption module. See figure 5 on page 15.
 * 
 * \param clk          40MHz board clock
 * \param load         raised when encryption should start
 * \param key          key to encrypt
 * \param plaintext    data to encrypt
 * \param done         raised with the encryption is finished
 * \param cyphertext   result of the encryption
 * 
 *   when load is asserted, takes the current key and plaintext
 *   generates cyphertext and asserts done when complete 11 cycles later
 * 
 *   See FIPS-197 with Nk = 4, Nb = 4, Nr = 10
 *
 *   The key and message are 128-bit values packed into an array of 16 bytes as
 *   shown below
 *        [127:120] [95:88] [63:56] [31:24]     S0,0    S0,1    S0,2    S0,3
 *        [119:112] [87:80] [55:48] [23:16]     S1,0    S1,1    S1,2    S1,3
 *        [111:104] [79:72] [47:40] [15:8]      S2,0    S2,1    S2,2    S2,3
 *        [103:96]  [71:64] [39:32] [7:0]       S3,0    S3,1    S3,2    S3,3
 *
 *   Equivalently, the values are packed into four words as given
 *        [127:96]  [95:64] [63:32] [31:0]      w[0]    w[1]    w[2]    w[3]
 */
module aes_core(input  logic         clk, 
                input  logic         load,
                input  logic [127:0] key, 
                input  logic [127:0] plaintext, 
                output logic         done, 
                output logic [127:0] cyphertext);

   logic [3:0] fsm_state, fsm_next;
   logic [127:0] cypher_state, cypher_next;
   logic [127:0] rkey, prkey, sbytes_in, srows_in, srows_out,
                 mcols_in, mcols_out;

   sub_bytes sb(sbytes_in, srows_in);
   shift_rows sr(srows_in, srows_out);
   mix_columns mc(mcols_in, mcols_out);
   next_round_key nrk(key, pkey, state, cur_key);

   // FSM state, cypher state, and round key registers
   always_ff @(posedge clk) begin
      fsm_state <= fsm_next;
      cypher_state <= cypher_next;
      prkey <= rkey;
   end

   // next state logic
   assign fsm_next = fsm_state == 4'd10 ? '0 : 4'h1;

   // cypher logic. see figure 5.
   assign sbytes_in = fsm_state == '0 ? '0 : cypher_state;
   assign mcols_in = fsm_state == '0 || fsm_state == 4'd10 ? '0 : srows_out;
   assign cypher_next = fsm_state == '0 ? plaintext ^ rkey
                      : fsm_state == 4'd10 ? srows_out ^ rkey
                      : mcols_out ^ rkey;
endmodule


/*
 * Implements the byte substitution algorithm as described in section 5.1.1
 * for entire state blocks.
 */
module sub_bytes(input logic [127:0] in, output logic [127:0] out);
   
   sub_word sw0(in[127:96], out[127:96]);
   sub_word sw1(in[95:64], out[95:64]);
   sub_word sw2(in[63:32], out[63:32]);
   sub_word sw2(in[31:0], out[31:0]);
endmodule

module sub_word(input logic [31:0] in, output logic [31:0] out);
   sbox sb03(in[31:24], out[31:24]);
   sbox sb13(in[23:16], out[23:16]);
   sbox sb23(in[15:8], out[15:8]);
   sbox sb33(in[7:0], out[7:0]);
endmodule

/**
 * Infamous AES byte substitutions with magic numbers
 * Section 5.1.1, Figure 7
 */
module sbox(input  logic [7:0] a, output logic [7:0] y);
            
  // sbox implemented as a ROM
  logic [7:0] sbox[0:255];

  initial   $readmemh("sbox.txt", sbox);
  assign y = sbox[a];
endmodule


/*
 * Even funkier action on columns
 * Section 5.1.3, Figure 9
 * Same operation performed on each of four columns
 */
module mix_columns(input  logic [127:0] a, output logic [127:0] y);

  mix_column mc0(a[127:96], y[127:96]);
  mix_column mc1(a[95:64],  y[95:64]);
  mix_column mc2(a[63:32],  y[63:32]);
  mix_column mc3(a[31:0],   y[31:0]);
endmodule


/*
 * Perform Galois field operations on bytes in a column
 * See EQ(4) from E. Ahmed et al, Lightweight Mix Columns Implementation for AES, AIC09
 * for this hardware implementation
 */
module mix_column(input  logic [31:0] a, output logic [31:0] y);
                      
   logic [7:0]       a0, a1, a2, a3, y0, y1, y2, y3, t0, t1, t2, t3, tmp;
        
   assign {a0, a1, a2, a3} = a;
   assign tmp = a0 ^ a1 ^ a2 ^ a3;
    
   galoismult gm0(a0^a1, t0);
   galoismult gm1(a1^a2, t1);
   galoismult gm2(a2^a3, t2);
   galoismult gm3(a3^a0, t3);

   assign y0 = a0 ^ tmp ^ t0;
   assign y1 = a1 ^ tmp ^ t1;
   assign y2 = a2 ^ tmp ^ t2;
   assign y3 = a3 ^ tmp ^ t3;
   assign y = {y0, y1, y2, y3};    
endmodule


/*
 * Multiply by x in GF(2^8) is a left shift
 * followed by an XOR if the result overflows
 * Uses irreducible polynomial x^8+x^4+x^3+x+1 = 00011011
 */
module galoismult(input  logic [7:0] a, output logic [7:0] y);

   logic [7:0] ashift;
    
   assign ashift = {a[6:0], 1'b0};
   assign y = a[7] ? (ashift ^ 8'b00011011) : ashift;
endmodule


/*
 * Apply the row shifting transformation specified in section 5.1.2
 */
module shift_rows(input logic [127:0] in, output logic [127:0] out);

   // top row: no permutation
   assign {out[127:120], out[95:88],   out[63:56],   out[31:23]}
        = {in[127:120],  in[95:88],    in[63:56],    in[31:23]};

   // second row: shift left by one
   assign {out[119:112], out[87:80],   out[55:48],   out[23:16]}
        = {in[87:80],    in[55:48],    in[23:16],    in[119:112]};

   // third row: shift left by two
   assign {out[111:104], out[79:72],   out[47:40],   out[15:8]}
        = {in[47:40],    in[15:8],     in[111:104],  in[79:72]};

   // fourth row: shift left by four
   assign {out[103:96],  out[71:64],   out[39:32],   out[7:0]}
        = {in[7:0],      in[103:96],   in[71:64],    in[39:32]};
endmodule


/**
 * Moudle to implement to Rcon array as described in section 5.2 paragraph 3.
 *
 * \param i    The index into Rcon divided by 4. See the second while loop in
 *             figure 11, specifically `Rcon[i/Nk]`
 * \param out  The value of Rcon[i/4].
 *
 * The array does not depend on ay inputs, so we implement it statically.
 * Also note that i can only take on values in the range 0...10, so we only
 * implement those values
 */
module rcon(input logic [3:0] i, output logic [31:0] out);

   logic [7:0] tmp;

   always_comb
      case (i)
      0: tmp = 'h1;
      1: tmp = 'h2;
      2: tmp = 'h4;
      3: tmp = 'h8;
      4: tmp = 'h10;
      5: tmp = 'h20;
      6: tmp = 'h40;
      7: tmp = 'h80;
      8: tmp = 'h1B;
      9: tmp = 'h36;
      10: tmp = 'h6C;
      default: tmp = 'hx;
      endcase

   assign out = {tmp, 24'b0};
endmodule


/**
 * create the next round key based off of the previous round key. purely
 * combinational. CF figure 11
 *
 * \param key     The cipher key
 * \param pkey    The previous round key
 * \param round   The round we are on
 * \param nkey    The next round key
 */
module next_round_key(input logic [127:0] key,
                      input logic [127:0] pkey,
                      input logic [3:0] round,
                      output logic [127:0] nkey);

   logic [7:0] __rcon, rot, sub;

   rcon rc(round, __rcon);
   assign rot = {pkey[31:24], pkey[7:0], pkey[15:8], pkey[23:16]};
   sub_word sw(rot, sub);

   always_comb begin
      if (round == '0) begin
         nkey = key;
      end else begin
         nkey[127:96] = pkey[127:96] ^ __rcon ^ sub;
         nkey[95:64] = pkey[95:64] ^ nkey[127:96];
         nkey[63:32] = pkey[63:32] ^ nkey[95:64];
         nkey[31:0] = pkey[31:0] ^ nkey[63:32];
      end
   end

endmodule





