//Provided HDMI_Text_controller_v1_0 for HDMI AXI4 IP 
//Fall 2024 Distribution

//Modified 3/10/24 by Zuofu
//Updated 11/18/24 by Zuofu


`timescale 1 ns / 1 ps

module hdmi_text_controller_v1_0 #
(
    // Parameters of Axi Slave Bus Interface S00_AXI
    // Modify parameters as necessary for access of full VRAM range

    parameter integer C_AXI_DATA_WIDTH	= 32,
    parameter integer C_AXI_ADDR_WIDTH	= 16 
)
(
    // Users to add ports here
    input logic reset_rtl_0,
    output logic hdmi_clk_n,
    output logic hdmi_clk_p,
    output logic [2:0] hdmi_tx_n,
    output logic [2:0] hdmi_tx_p,
    
    output logic vsync_out,
    // Add these alongside your hdmi_clk_p, etc.
    output logic [16:0] sprite_rom_addr,
    input  logic [3:0]  sprite_rom_data,
    
    output logic [16:0] fbr_addr,
    input  logic [3:0] fbr_data,
    
    output logic [16:0] fbw_addr,
    output logic [3:0] fbw_data,
    output logic fbw_we,

    // User ports ends
    // Do not modify the ports beyond this line


    // Ports of Axi Slave Bus Interface AXI
    input logic  axi_aclk,
    input logic  axi_aresetn,
    input logic [C_AXI_ADDR_WIDTH-1 : 0] axi_awaddr,
    input logic [2 : 0] axi_awprot,
    input logic  axi_awvalid,
    output logic  axi_awready,
    input logic [C_AXI_DATA_WIDTH-1 : 0] axi_wdata,
    input logic [(C_AXI_DATA_WIDTH/8)-1 : 0] axi_wstrb,
    input logic  axi_wvalid,
    output logic  axi_wready,
    output logic [1 : 0] axi_bresp,
    output logic  axi_bvalid,
    input logic  axi_bready,
    input logic [C_AXI_ADDR_WIDTH-1 : 0] axi_araddr,
    input logic [2 : 0] axi_arprot,
    input logic  axi_arvalid,
    output logic  axi_arready,
    output logic [C_AXI_DATA_WIDTH-1 : 0] axi_rdata,
    output logic [1 : 0] axi_rresp,
    output logic  axi_rvalid,
    input logic  axi_rready
);

//additional logic variables as necessary to support VGA, and HDMI modules.
logic [31:0] frameCt;
logic [31:0] slv_regs_data[512];
logic [9:0] drawX, drawY;
// Instantiation of Axi Bus Interface AXI
hdmi_text_controller_v1_0_AXI # ( 
    .C_S_AXI_DATA_WIDTH(C_AXI_DATA_WIDTH),
    .C_S_AXI_ADDR_WIDTH(C_AXI_ADDR_WIDTH)
) hdmi_text_controller_v1_0_AXI_inst (
    .S_AXI_ACLK(axi_aclk),
    .S_AXI_ARESETN(axi_aresetn),
    .S_AXI_AWADDR(axi_awaddr),
    .S_AXI_AWPROT(axi_awprot),
    .S_AXI_AWVALID(axi_awvalid),
    .S_AXI_AWREADY(axi_awready),
    .S_AXI_WDATA(axi_wdata),
    .S_AXI_WSTRB(axi_wstrb),
    .S_AXI_WVALID(axi_wvalid),
    .S_AXI_WREADY(axi_wready),
    .S_AXI_BRESP(axi_bresp),
    .S_AXI_BVALID(axi_bvalid),
    .S_AXI_BREADY(axi_bready),
    .S_AXI_ARADDR(axi_araddr),
    .S_AXI_ARPROT(axi_arprot),
    .S_AXI_ARVALID(axi_arvalid),
    .S_AXI_ARREADY(axi_arready),
    .S_AXI_RDATA(axi_rdata),
    .S_AXI_RRESP(axi_rresp),
    .S_AXI_RVALID(axi_rvalid),
    .S_AXI_RREADY(axi_rready),
    .frameCt(frameCt),
    .drawX(drawX),
    .drawY(drawY),
    .slv_regs_data(slv_regs_data)
);


//Instiante clocking wizard, VGA sync generator modules, and VGA-HDMI IP here. For a hint, refer to the provided
//top-level from the previous lab. You should get the IP to generate a valid HDMI signal (e.g. blue screen or gradient)
//prior to working on the text drawing.
    logic clk_25MHz, clk_125MHz;
    logic locked;

    logic hsync, vsync, vde;
    assign vsync_out = vsync;
    logic [3:0] red, green, blue;
    logic reset_ah;
    
    assign reset_ah = ~reset_rtl_0;

// clk wizard for hdmi // ip catalog
    clk_wiz_0 clk_wiz (
        .clk_out1(clk_25MHz),
        .clk_out2(clk_125MHz),
        .reset(reset_ah),
        .locked(locked),
        .clk_in1(axi_aclk)
    );
    
// vga sync signal generator // below
    vga_controller vga (
        .pixel_clk(clk_25MHz),
        .reset(reset_ah),
        .hs(hsync),
        .vs(vsync),
        .active_nblank(vde),
        .drawX(drawX),
        .drawY(drawY)
    ); 
    
// real digital vga to hdmi // ip catalog
    hdmi_tx_0 vga_to_hdmi (
        //Clocking and Reset
        .pix_clk(clk_25MHz),
        .pix_clkx5(clk_125MHz),
        .pix_clk_locked(locked),
        .rst(reset_ah),
        //Color and Sync Signals
        .red(red),
        .green(green),
        .blue(blue),
        .hsync(hsync),
        .vsync(vsync),
        .vde(vde),
        
        //aux Data (unused)
        .aux0_din(4'b0),
        .aux1_din(4'b0),
        .aux2_din(4'b0),
        .ade(1'b0),
        
        //Differential outputs
        .TMDS_CLK_P(hdmi_clk_p),          
        .TMDS_CLK_N(hdmi_clk_n),          
        .TMDS_DATA_P(hdmi_tx_p),         
        .TMDS_DATA_N(hdmi_tx_n)          
    );
       
frame_counter frame_counter_inst(.vsync(vsync), 
                                 .reset(reset_ah), 
                                 .counter(frameCt));


// color mapper
color_mapper color_instance(
    .clk(axi_aclk),
    .reset(reset_ah),
    .DrawX(drawX),
    .DrawY(drawY),
    .slv_regs(slv_regs_data), 
    .vde(vde),
    .frameCt(frameCt),
    .vsync(vsync),
    .Red(red),
    .Green(green),
    .Blue(blue),
    .sprite_rom_addr(sprite_rom_addr),
    .sprite_rom_data(sprite_rom_data),
    .fbw_addr(fbw_addr),
    .fbw_data(fbw_data),
    .fbw_we(fbw_we),
    .fbr_addr(fbr_addr),
    .fbr_data(fbr_data)
);

endmodule

module color_mapper ( 
    input  logic          clk,
    input  logic          reset,
    input  logic [9:0]    DrawX, DrawY,
    input  logic [31:0]   slv_regs[512],
    input  logic          vde,
    input  logic          vsync,
    input  logic [31:0]   frameCt,
    
    output logic [16:0]   sprite_rom_addr,
    input  logic [3:0]    sprite_rom_data,
    
    output logic [3:0]    Red, Green, Blue,
    
    output logic [16:0]   fbw_addr,
    output logic [3:0]    fbw_data,
    output logic          fbw_we,
    
    output logic [16:0]   fbr_addr,
    input  logic [3:0]    fbr_data
);

    logic [8:0] ScaledX, ScaledY;
    assign ScaledX = DrawX[9:1]; 
    assign ScaledY = DrawY[9:1];
    
    logic [8:0] SpriteX, SpriteY;
    assign SpriteX = slv_regs[0][8:0];
    assign SpriteY = slv_regs[1][8:0];
    
    logic [8:0] SpriteSize = 9'd16;
        
    // Allocating mmio for player sprite
    logic [16:0] sprite_base_addr;
    assign sprite_base_addr = slv_regs[3][16:0]; 

	// enemy reg malloc
    localparam NUM_ENEMIES = 50;
    logic [8:0]  EnemyX [NUM_ENEMIES];
    logic [8:0]  EnemyY [NUM_ENEMIES];
    logic [16:0] EnemyAddr [NUM_ENEMIES];

    always_comb begin
        for (int i = 0; i < NUM_ENEMIES; i++) begin
            EnemyAddr[i] = slv_regs[50 + (i*2)][16:0];
            EnemyX[i] = slv_regs[51 + (i*2)][24:16];
            EnemyY[i] = slv_regs[51 + (i*2)][8:0];   
        end
    end
    
    // Bullet malloc and logic
    localparam NUM_BULLETS = 8;
    
    logic [8:0] BulletX [NUM_BULLETS];
    logic [8:0] BulletY [NUM_BULLETS];
    logic       bullet_active [NUM_BULLETS];
    logic [2:0] bullet_dir [NUM_BULLETS];

    always_comb begin
        for (int i = 0; i < NUM_BULLETS; i++) begin
            BulletX[i]       = slv_regs[15 + (i*3)][8:0];
            BulletY[i]       = slv_regs[16 + (i*3)][8:0];
            bullet_active[i] = slv_regs[17 + (i*3)][0];
            bullet_dir[i]    = slv_regs[17 + (i*3)][3:1];
        end
    end
    
    logic [2:0] game_state;
    assign game_state = slv_regs[5][2:0];
            
    
    // FRAME BUFFER FSM
    
    // States
    typedef enum logic [3:0] {
        STATE_WAIT_VSYNC,      // Init
        STATE_BG_BORDER,       // Clearing & Borders
        STATE_HUD,             // Health, Score, Wave
        STATE_ENEMIES_REQ,     // Enemy & Enemy Projectiles
        STATE_PLAYER_REQ,      // Player
        STATE_P_BULLETS,       // Player Bullets
        STATE_E_BULLETS,       // Enemy Bullets
        STATE_BRAM_WAIT,       // Wait State
        STATE_WRITE_PIXEL,     // Write State
        STATE_STATIC_DISPLAY
    } state_t;

    state_t state, return_state;

    // counters for tracking loops
    logic [16:0] screen_idx;  // 0 to 76799 for the whole screen
    logic [8:0]  cur_x;       // 0-319 320x240 screen
    logic [8:0]  cur_y;       // 0-239
    logic [5:0]  enemy_idx;   // 0 to 49 for enemies
    logic [7:0]  sprite_idx;  // 0 to 255 for a 16x16 sprite
    logic [3:0]  bullet_idx;  // 0 to 7 for player bullets
    logic [4:0]  e_bullet_idx;// 0 to 31 for enemy bullets (only 25 but)
    logic [5:0]  hud_idx;     // HUD element tracker
    logic [7:0]  static_idx;  // 0 to 255 for a static sprite from reg

    // logic signals
    logic vsync_prev;
    
    logic [8:0] target_x, target_y;
    logic [16:0] rom_base;
    logic [8:0] px_x, px_y;
    
    logic [31:0] static_xy;
    logic [31:0] static_id;
    
    logic [31:0] eb_xy;
    logic [31:0] eb_id;

    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            state <= STATE_WAIT_VSYNC;
            vsync_prev <= 1'b0;
            screen_idx <= 17'd0;
            enemy_idx <= 6'd0;
            sprite_idx <= 8'd0;
            bullet_idx <= 4'd0;
            fbw_we <= 1'b0;
            e_bullet_idx <= 5'd0;
        end else begin
            // rising edge detection
            vsync_prev <= vsync; 
            fbw_we <= 1'b0; 

            case (state)
                // wait state
                STATE_WAIT_VSYNC: begin
                    if (vsync == 1'b1 && vsync_prev == 1'b0) begin
                        screen_idx <= 17'd0;
                        cur_x <= 9'd0;
                        cur_y <= 9'd0;
                        state <= STATE_BG_BORDER;
                    end
                end

                // clear screen and draw border
                STATE_BG_BORDER: begin
                    fbw_we <= 1'b1;
                    fbw_addr <= screen_idx;
                    
                    // Border only during gameplay (game state 3)
                    if (game_state == 3'd3 && cur_x >= 9'd1 && cur_x <= 9'd318 && cur_y >= 9'd16 && cur_y <= 9'd231) begin
                        if (cur_x <= 9'd2 || cur_x >= 9'd317 || cur_y <= 9'd17 || cur_y >= 9'd230) begin
                            fbw_data <= 4'h4; //border color (pick cycle one)
                        end else begin
                            fbw_data <= 4'h0; // black
                        end
                    end else begin
                        fbw_data <= 4'h0; 
                    end
                    
                    // done drawing
                    if (screen_idx == 17'd76799) begin
                        
                        // Route based on Game State
                        if (game_state == 3'd0 || game_state == 3'd4 || game_state == 3'd5) begin //static screen drawer
                            state <= STATE_STATIC_DISPLAY;
                            static_idx <= 8'd0;
                            sprite_idx <= 8'd0;
                        end else begin
                            // in gameplay
                            state <= STATE_HUD; 
                            sprite_idx <= 8'd0;
                            hud_idx <= 6'd0; 
                        end
                        
                    end else begin
                        screen_idx <= screen_idx + 1;
                        if (cur_x == 9'd319) begin
                            cur_x <= 9'd0;
                            cur_y <= cur_y + 1;
                        end else begin
                            cur_x <= cur_x + 1;
                        end
                    end
                end
                
                // menu drawing from static regs
                STATE_STATIC_DISPLAY: begin
                    if (static_idx < 8'd150) begin // 150 sprites (300 registers)
                        
                        static_xy = slv_regs[212 + (static_idx * 2)];
                        static_id = slv_regs[212 + (static_idx * 2) + 1];

                        // draw the 16x16 sprite if not -1
                        if (static_id != 32'hFFFFFFFF) begin
                            
                            target_x = static_xy[24:16];
                            target_y = static_xy[8:0];
							
							//standard drawing procedure
                            sprite_rom_addr <= static_id[16:0] + sprite_idx[3:0] + (sprite_idx[7:4] * 288);
                            
                            fbw_addr <= (target_x + sprite_idx[3:0]) + ((target_y + sprite_idx[7:4]) * 320);

                            return_state <= STATE_STATIC_DISPLAY;
                            state <= STATE_BRAM_WAIT;

                        end else begin
                            static_idx <= static_idx + 1;
                        end
                        
                    end else begin
                        // done drawing
                        state <= STATE_WAIT_VSYNC;
                    end
                end

                // HUD
                STATE_HUD: begin
                    // determine total elements needed to be drawn
                    if (hud_idx < 6'd30) begin 
                        
                        case (hud_idx)
                            // LIVES
                            6'd0: begin target_x = 9'd8;  target_y = 9'd0; rom_base = 17'd55344;  end //L
                            6'd1: begin target_x = 9'd16; target_y = 9'd0; rom_base = 17'd55296;  end //I
                            6'd2: begin target_x = 9'd24; target_y = 9'd0; rom_base = 17'd55504;  end //V
                            6'd3: begin target_x = 9'd32; target_y = 9'd0; rom_base = 17'd50912;  end //E
                            6'd4: begin target_x = 9'd40; target_y = 9'd0; rom_base = 17'd55456;  end //S
                            
                            // life sprites
                            6'd5, 6'd6, 6'd7, 6'd8, 6'd9: begin
                                target_x = 9'd56 + ((hud_idx - 6'd5) * 9'd8); 
                                target_y = 9'd0;
                                rom_base = 17'd23136;
                            end

                            // SCORE
                            6'd10: begin target_x = 9'd192; target_y = 9'd0; rom_base = 17'd55456;  end //S
                            6'd11: begin target_x = 9'd200; target_y = 9'd0; rom_base = 17'd50880;  end //C
                            6'd12: begin target_x = 9'd208; target_y = 9'd0; rom_base = 17'd55392;  end //O
                            6'd13: begin target_x = 9'd216; target_y = 9'd0; rom_base = 17'd55440;  end //R
                            6'd14: begin target_x = 9'd224; target_y = 9'd0; rom_base = 17'd50912;  end //E

                            // actual score numbers
                            6'd15: begin target_x = 9'd240; target_y = 9'd0; rom_base = 17'd50688 + (slv_regs[9][3:0]   * 16); end // 100 Millions
                            6'd16: begin target_x = 9'd248; target_y = 9'd0; rom_base = 17'd50688 + (slv_regs[8][31:28] * 16); end // 10 Millions
                            6'd17: begin target_x = 9'd256; target_y = 9'd0; rom_base = 17'd50688 + (slv_regs[8][27:24] * 16); end // Millions
                            6'd18: begin target_x = 9'd264; target_y = 9'd0; rom_base = 17'd50688 + (slv_regs[8][23:20] * 16); end // 100 Thousands
                            6'd19: begin target_x = 9'd272; target_y = 9'd0; rom_base = 17'd50688 + (slv_regs[8][19:16] * 16); end // 10 Thousands
                            6'd20: begin target_x = 9'd280; target_y = 9'd0; rom_base = 17'd50688 + (slv_regs[8][15:12] * 16); end // Thousands
                            6'd21: begin target_x = 9'd288; target_y = 9'd0; rom_base = 17'd50688 + (slv_regs[8][11:8]  * 16); end // Hundreds
                            6'd22: begin target_x = 9'd296; target_y = 9'd0; rom_base = 17'd50688 + (slv_regs[8][7:4]   * 16); end // Tens
                            6'd23: begin target_x = 9'd304; target_y = 9'd0; rom_base = 17'd50688 + (slv_regs[8][3:0]   * 16); end // Ones

                            // WAVE
                            6'd24: begin target_x = 9'd10; target_y = 9'd232; rom_base = 17'd46304; end //W
                            6'd25: begin target_x = 9'd18; target_y = 9'd232; rom_base = 17'd41632; end //A
                            6'd26: begin target_x = 9'd26; target_y = 9'd232; rom_base = 17'd46288; end //V
                            6'd27: begin target_x = 9'd34; target_y = 9'd232; rom_base = 17'd41696; end //E

                            // actual wave number
                            6'd28: begin target_x = 9'd50; target_y = 9'd232; rom_base = 17'd41472 + (slv_regs[10][7:4] * 16); end // Wave Tens
                            6'd29: begin target_x = 9'd58; target_y = 9'd232; rom_base = 17'd41472 + (slv_regs[10][3:0] * 16); end // Wave Ones

                            default: begin target_x = 9'd400; target_y = 9'd400; rom_base = 0; end
                        endcase

                        // dead life sprite, skip drawing 
                        if (hud_idx >= 6'd5 && hud_idx <= 6'd9 && (hud_idx - 6'd5) >= slv_regs[2][4:0]) begin
                            
                            hud_idx <= hud_idx + 1;
                            state <= STATE_HUD;
                            
                        end else begin
                            
                            // standard drawing procedure
                            sprite_rom_addr <= rom_base + sprite_idx[3:0] + (sprite_idx[7:4] * 288);
                            
                            fbw_addr <= (target_x + sprite_idx[3:0]) + ((target_y + sprite_idx[7:4]) * 320);

                            return_state <= STATE_HUD;
                            state <= STATE_BRAM_WAIT;
                        end

                    end else begin
                        // done drawing
                        state <= STATE_ENEMIES_REQ;
                        enemy_idx <= 6'd0;
                        sprite_idx <= 8'd0;
                    end
                end

                // enemy drawing
                STATE_ENEMIES_REQ: begin
                    if (enemy_idx < 50) begin
                        if (EnemyAddr[enemy_idx] != 17'h1FFFF) begin // not -1
                            
                            // standard drawing procedure
                            sprite_rom_addr <= EnemyAddr[enemy_idx] + sprite_idx[3:0] + (sprite_idx[7:4] * 288);
                            
                            fbw_addr <= (EnemyX[enemy_idx] + sprite_idx[3:0]) + ((EnemyY[enemy_idx] + sprite_idx[7:4]) * 320);
                            
                            return_state <= STATE_ENEMIES_REQ;
                            state <= STATE_BRAM_WAIT;
                            
                        end else begin
                            enemy_idx <= enemy_idx + 1; 
                        end
                    end else begin
						//done drawing
                        state <= STATE_PLAYER_REQ; 
                        sprite_idx <= 8'd0;
                    end
                end

                // draw player
                STATE_PLAYER_REQ: begin
                    // get pixel from rom
                    sprite_rom_addr <= sprite_base_addr + sprite_idx[3:0] + (sprite_idx[7:4] * 288);
                    
                    // determine where to paste sticker in frame buffer
                    fbw_addr <= (SpriteX + sprite_idx[3:0]) + ((SpriteY + sprite_idx[7:4]) * 320);
                    
                    // wait
                    return_state <= STATE_PLAYER_REQ;
                    state <= STATE_BRAM_WAIT;
                end

                // hardcoded player bullets
                STATE_P_BULLETS: begin
                    if (bullet_idx < 8) begin 
                        if (bullet_active[bullet_idx] == 1'b1) begin
                            
                            // bullet is 6 pixels in a dir
                            if (sprite_idx < 8'd6) begin
                                
                                // determine pixel offsets based on bullet direction using math
                                case (bullet_dir[bullet_idx])
                                    3'd0, 3'd4: begin px_x = 9'd2; px_y = sprite_idx; end 
                                    3'd2, 3'd6: begin px_x = sprite_idx; px_y = 9'd2; end 
                                    3'd3, 3'd7: begin px_x = sprite_idx; px_y = sprite_idx; end 
                                    3'd1, 3'd5: begin px_x = sprite_idx; px_y = 9'd5 - sprite_idx; end 
                                    default:    begin px_x = 9'd0; px_y = 9'd0; end
                                endcase
                                
                                fbw_we <= 1'b1;
                                fbw_data <= 4'h1; // white bullets
                                fbw_addr <= (BulletX[bullet_idx] + px_x) + ((BulletY[bullet_idx] + px_y) * 320);
                                
                                sprite_idx <= sprite_idx + 1;
                                
                            end else begin
                                sprite_idx <= 8'd0;
                                bullet_idx <= bullet_idx + 1;
                            end
                            
                        end else begin
                            bullet_idx <= bullet_idx + 1;
                        end
                        
                    end else begin
                        // done drawing
                        state <= STATE_E_BULLETS;
                        e_bullet_idx <= 5'd0;
                        sprite_idx <= 8'd0;
                    end
                end
                
                // enemy projectiles
                STATE_E_BULLETS: begin
                    if (e_bullet_idx < 5'd25) begin 
                        
                        // starts at reg150
                        eb_xy = slv_regs[150 + (e_bullet_idx * 3)];
                        eb_id = slv_regs[150 + (e_bullet_idx * 3) + 2];

                        // not -1
                        if (eb_id != 32'hFFFFFFFF) begin
                            
                            // grab from C
                            target_x = eb_xy[24:16];
                            target_y = eb_xy[8:0];

                            // standard drawing procedure
                            sprite_rom_addr <= eb_id[16:0] + sprite_idx[3:0] + (sprite_idx[7:4] * 288);
                            
                            fbw_addr <= (target_x + sprite_idx[3:0]) + ((target_y + sprite_idx[7:4]) * 320);

                            return_state <= STATE_E_BULLETS;
                            state <= STATE_BRAM_WAIT;

                        end else begin
                            e_bullet_idx <= e_bullet_idx + 1;
                        end
                        
                    end else begin
                        // done drawing
                        state <= STATE_WAIT_VSYNC;
                    end
                end
                
                // BRAM delay wait state
                STATE_BRAM_WAIT: begin
                    // do nothing for 1 clock cycle 
                    state <= STATE_WRITE_PIXEL;
                end

                STATE_WRITE_PIXEL: begin
                    // writing to frame buffer, check transparency pixel
                    if (sprite_rom_data != 4'h0) begin
                        fbw_we <= 1'b1;
                        fbw_data <= sprite_rom_data;
                    end
                    
                    if (sprite_idx == 8'd255) begin
                        sprite_idx <= 8'd0;
                        
                        // Branch back to whatever state we were at
                        if (return_state == STATE_STATIC_DISPLAY) begin
                            static_idx <= static_idx + 1;
                            state <= return_state;
                        end
                        else if (return_state == STATE_E_BULLETS) begin 
                            e_bullet_idx <= e_bullet_idx + 1;
                            state <= return_state;
                        end
                        else if (return_state == STATE_ENEMIES_REQ) begin
                            enemy_idx <= enemy_idx + 1;
                            state <= return_state;
                        end 
                        else if (return_state == STATE_HUD) begin
                            hud_idx <= hud_idx + 1;
                            state <= return_state; 
                        end 
                        else if (return_state == STATE_PLAYER_REQ) begin
                            state <= STATE_P_BULLETS;
                            bullet_idx <= 4'd0;
                            sprite_idx <= 8'd0;
                        end
                    end else begin
                        sprite_idx <= sprite_idx + 1;
                        state <= return_state; 
                    end
                end

            endcase
        end
    end
    
    
    
    
    assign fbr_addr = ScaledX + (ScaledY * 320);

    logic [3:0] palette_red, palette_green, palette_blue;
    
    logic cycle_ena;
    assign cycle_ena = (game_state == 3'd3);
    
    robotronv4_palette palette_inst (
        .index(fbr_data), 
        .red(palette_red),     
        .green(palette_green), 
        .blue(palette_blue),
        .cycle_ena(cycle_ena),
        .frame_ct(frameCt)
    );

    always_comb begin
        if (vde == 1'b0) begin
            // MUST OUTPUT BLACK DURING BLANKING
            Red   = 4'h0;
            Green = 4'h0;
            Blue  = 4'h0;
        end 
        else begin
            Red   = palette_red;
            Green = palette_green;
            Blue  = palette_blue;
        end
    end
endmodule


module frame_counter (input logic vsync, reset,
                      output logic [31:0] counter);          
         always_ff @(posedge vsync or posedge reset) begin
         if (reset)
            counter <= 32'd0;
         else
            counter <= counter + 32'd1;
         end             
                      
endmodule

