module robotronv4_palette (
	input logic [3:0] index,
	output logic [3:0] red, green, blue,
	input logic cycle_ena,
	input logic [31:0] frame_ct
);

localparam [0:15][11:0] palette = {
	{4'h0, 4'h0, 4'h0},  // 0: Black
	{4'hF, 4'hF, 4'hF},  // 1: White
	{4'hE, 4'h4, 4'h0},  // 2: Orange-Red
	{4'h0, 4'hF, 4'h0},  // 3: Green
	{4'h0, 4'h0, 4'hF},  // 4: Blue
	{4'h8, 4'h0, 4'hF},  // 5: Purple
	{4'hF, 4'h0, 4'h0},  // 6: Red
	{4'hF, 4'hF, 4'h0},  // 7: Yellow
	{4'h9, 4'h9, 4'h9},  // 8: Gray
	{4'hE, 4'h0, 4'hF},  // 9: Magenta
	{4'h2, 4'h0, 4'hA},  // 10: Dark Purple
	{4'hF, 4'h0, 4'h0},  // 11: Red
	{4'hF, 4'hF, 4'hF},  // 12: White
	{4'h0, 4'h0, 4'hC},  // 13: Dark Blue
	{4'h0, 4'h0, 4'h0},  // 14: Black
	{4'h0, 4'h0, 4'h0}   // 15: Black
};

// Extended color definitions for 8-color cycling
localparam [11:0] COLOR_RED        = {4'hF, 4'h0, 4'h0};
localparam [11:0] COLOR_GREEN      = {4'h0, 4'hF, 4'h0};
localparam [11:0] COLOR_BLUE       = {4'h0, 4'h0, 4'hF};
localparam [11:0] COLOR_WHITE      = {4'hF, 4'hF, 4'hF};
localparam [11:0] COLOR_YELLOW     = {4'hF, 4'hF, 4'h0};
localparam [11:0] COLOR_PURPLE     = {4'h8, 4'h0, 4'hF};
localparam [11:0] COLOR_CYAN       = {4'h0, 4'hF, 4'hF};
localparam [11:0] COLOR_MAGENTA    = {4'hF, 4'h0, 4'hF};
localparam [11:0] COLOR_ORANGE     = {4'hF, 4'h8, 4'h0};
localparam [11:0] COLOR_DARK_BLUE  = {4'h0, 4'h0, 4'hC};
localparam [11:0] COLOR_DARK_PURPLE = {4'h2, 4'h0, 4'hA};
localparam [11:0] COLOR_ORANGE_RED = {4'hE, 4'h4, 4'h0};
localparam [11:0] COLOR_GRAY       = {4'h9, 4'h9, 4'h9};

logic [11:0] current_color;

always_comb begin
	if (cycle_ena) begin
		case (index)
			// Red indices: 6, 11
			4'd6, 4'd11: begin
				case (frame_ct[5:3])
					3'b000: current_color = COLOR_RED;
					3'b001: current_color = COLOR_ORANGE;
					3'b010: current_color = COLOR_YELLOW;
					3'b011: current_color = COLOR_MAGENTA;
					3'b100: current_color = COLOR_PURPLE;
					3'b101: current_color = COLOR_ORANGE_RED;
					3'b110: current_color = COLOR_WHITE;
					3'b111: current_color = COLOR_GREEN;
				endcase
			end
			
			// Blue indices: 4
			4'd4: begin
				case (frame_ct[5:3])
					3'b000: current_color = COLOR_BLUE;
					3'b001: current_color = COLOR_CYAN;
					3'b010: current_color = COLOR_DARK_BLUE;
					3'b011: current_color = COLOR_PURPLE;
					3'b100: current_color = COLOR_MAGENTA;
					3'b101: current_color = COLOR_WHITE;
					3'b110: current_color = COLOR_YELLOW;
					3'b111: current_color = COLOR_GREEN;
				endcase
			end
			
			// White indices: 1, 12
			4'd1, 4'd12: begin
				case (frame_ct[5:3])
					3'b000: current_color = COLOR_WHITE;
					3'b001: current_color = COLOR_YELLOW;
					3'b010: current_color = COLOR_CYAN;
					3'b011: current_color = COLOR_GRAY;
					3'b100: current_color = COLOR_RED;
					3'b101: current_color = COLOR_GREEN;
					3'b110: current_color = COLOR_BLUE;
					3'b111: current_color = COLOR_MAGENTA;
				endcase
			end
			
			// All other indices use base palette
			default: current_color = palette[index];
		endcase
	end else begin
		current_color = palette[index];
	end
end

assign {red, green, blue} = current_color;

endmodule