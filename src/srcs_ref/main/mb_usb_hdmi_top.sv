//-------------------------------------------------------------------------
//    mb_usb_hdmi_top.sv                                                 --
//    Zuofu Cheng                                                        --
//    2-29-24                                                            --
//    10-14-25                                                           --
//                                                                       --
//    Fall 2025 Distribution                                             --
//                                                                       --
//    For use with ECE 385 USB + HDMI                                    --
//    University of Illinois ECE Department                              --
//-------------------------------------------------------------------------
//640 x 480
//320 x 240 each pixel 4 bits

module mb_usb_hdmi_top(
    input logic Clk,
    input logic reset_rtl_0,
    
    //USB signals
    input logic [0:0] gpio_usb_int_tri_i,
    output logic gpio_usb_rst_tri_o,
    input logic usb_spi_miso,
    output logic usb_spi_mosi,
    output logic usb_spi_sclk,
    output logic usb_spi_ss,
    
    //UART
    input logic uart_rtl_0_rxd,
    output logic uart_rtl_0_txd,
    
    //HDMI
    output logic hdmi_tmds_clk_n,
    output logic hdmi_tmds_clk_p,
    output logic [2:0]hdmi_tmds_data_n,
    output logic [2:0]hdmi_tmds_data_p,
        
    //HEX displays
    output logic [7:0] hex_segA,
    output logic [3:0] hex_gridA,
    output logic [7:0] hex_segB,
    output logic [3:0] hex_gridB
);
    
    logic [31:0] keycode0_gpio, keycode1_gpio;
    logic clk_25MHz, clk_125MHz, clk, clk_100MHz;
    logic locked;
    logic [9:0] drawX, drawY, ballxsig, ballysig, ballsizesig;

    logic hsync, vsync, vde;
    logic [3:0] red, green, blue;
    logic reset_ah;
    
    assign reset_ah = reset_rtl_0;
    
    logic [16:0] rom_addr_wire;
    logic [3:0]  rom_data_wire;
        
    logic frame_read;
    
    
    //Keycode HEX drivers
    hex_driver HexA (
        .clk(Clk),
        .reset(reset_ah),
        .in({keycode0_gpio[31:28], keycode0_gpio[27:24], keycode0_gpio[23:20], keycode0_gpio[19:16]}),
        .hex_seg(hex_segA),
        .hex_grid(hex_gridA)
    );
    
    hex_driver HexB (
        .clk(Clk),
        .reset(reset_ah),
        .in({keycode0_gpio[15:12], keycode0_gpio[11:8], keycode0_gpio[7:4], keycode0_gpio[3:0]}),
        .hex_seg(hex_segB),
        .hex_grid(hex_gridB)
    );
    logic [16:0] fbr_addr;
    logic [3:0] fbr_data;
    logic [16:0] fbw_addr;
    logic [3:0] fbw_data;
    logic fbw_we;
    mb_robot mb_block_i (
        .clk_100MHz(Clk),
        
        .gpio_usb_int_tri_i(gpio_usb_int_tri_i),
        .gpio_usb_keycode_0_tri_o(keycode0_gpio),
        .gpio_usb_keycode_1_tri_o(keycode1_gpio),
        .gpio_usb_rst_tri_o(gpio_usb_rst_tri_o),
        
        .reset_rtl_0(~reset_ah), //Block designs expect active low reset, all other modules are active high
        
        .uart_rtl_0_rxd(uart_rtl_0_rxd),
        .uart_rtl_0_txd(uart_rtl_0_txd),
        
        .usb_spi_miso(usb_spi_miso),
        .usb_spi_mosi(usb_spi_mosi),
        .usb_spi_sclk(usb_spi_sclk),
        .usb_spi_ss(usb_spi_ss),
        
        .HDMI_0_tmds_clk_n(hdmi_tmds_clk_n),
        .HDMI_0_tmds_clk_p(hdmi_tmds_clk_p),
        .HDMI_0_tmds_data_n(hdmi_tmds_data_n),
        .HDMI_0_tmds_data_p(hdmi_tmds_data_p),
        
        .sprite_rom_addr_0(rom_addr_wire),
        .sprite_rom_data_0(rom_data_wire),
        
        .vsync_out_0(vsync),
        
        .fbr_addr_0(fbr_addr), // output
        .fbr_data_0(fbr_data), // input
        .fbw_addr_0(fbw_addr), // output 
        .fbw_data_0(fbw_data), // output
        .fbw_we_0(fbw_we) //output
    );
    
    blk_mem_gen_0 sprite_rom_inst (
        .clka(Clk), 
        .addra(rom_addr_wire),
        .douta(rom_data_wire)
    );
    

    
    
    logic [16:0] fb1_addr;
    logic [3:0] fb1_din;
    logic [3:0] fb1_dout;
    logic wea1;
    
    logic [16:0] fb2_addr;
    logic [3:0] fb2_din;
    logic [3:0] fb2_dout;
    logic wea2;
    
    
    frame_buffer_1 fb1_inst ( // needs separate ena from ip 
        .addra(fb1_addr),
        .clka(Clk),
        .dina(fb1_din),
        .douta(fb1_dout),
        .ena(1'b1), // assign to ip output
        .wea(wea1 & fbw_we)
    );
    
    frame_buffer_2 fb2_inst ( // needs separate ena from ip
        .addra(fb2_addr),
        .clka(Clk),
        .dina(fb2_din),
        .douta(fb2_dout),
        .ena(1'b1), // assign to ip output
        .wea(wea2 & fbw_we)
    );
    
    assign fb1_addr = wea1 ? fbw_addr : fbr_addr;
    assign fb2_addr = wea2 ? fbw_addr : fbr_addr;
    
    assign fb1_din = fbw_data;
    assign fb2_din = fbw_data;
    
    assign fbr_data = wea1 ? fb2_dout : fb1_dout;
    
    logic vsync_prev;
    always_ff @(posedge Clk) begin
         if (reset_ah) begin
            wea1 <= 1'b0;
            wea2 <= 1'b1;
            vsync_prev <= 1'b0;
         end
         else begin
            vsync_prev <= vsync;
            // Only swap exactly once when vsync goes high
            if (vsync == 1'b1 && vsync_prev == 1'b0) begin
                wea1 <= ~wea1;
                wea2 <= ~wea2;
            end
         end
    end     
    
    
endmodule