/////////////////////////////////////////////////////////////////////////////////////
// File that contains the definition of the graphic interface management
/////////////////////////////////////////////////////////////////////////////////////
#ifndef GRAPHIC_HPP
#define GRAPHIC_HPP
#include "utility.hpp"



/////////////////////////////////////////////////////////////////////////////////////
// Defining all the constants link to the graphic interface management
/////////////////////////////////////////////////////////////////////////////////////






/////////////////////////////////////////////////////////////////////////////////////
// Enum that represents the type the name of the different buttons in the graphic interface
/////////////////////////////////////////////////////////////////////////////////////
#define NUMBER_OF_BUTTONS 4
enum {
    QUIT_STOCK_MARKET,
    CONNECT_CLIENT, 
    LOGIN_CLIENT,
    CREATE_CLIENT
};
// Enum that represents the state of each buttons 
enum {UNACTIVE, ACTIVE};
// UNACTIVE : the button is not active, it's not been clicked
// ACTIVE : the button is active, it's been clicked
/////////////////////////////////////////////////////////////////////////////////////
// Button is a structure that represents the buttons (the area to click on and the activity state) in the game
/////////////////////////////////////////////////////////////////////////////////////
typedef struct Button{
    SDL_Rect rect; // the rectangle of the button
    int state; // the state of the button : UNACTIVE or ACTIVE
    // the type will be given by the position in the enum of buttons
    // for example Buttons[1] correspond to the button "CONNECT_CLIENT" since in the enum CONNECT_CLIENT=5
} Button;



///////////////////////////////////////////////////////////////////////////////////// 
// Class to manage the buttons in the game
/////////////////////////////////////////////////////////////////////////////////////
class Graphic_Buttons
{
private:
    std::vector<Button> Buttons;
    int Number_of_Buttons;

public:
    // constructor (initialize the buttons)
    Graphic_Buttons(const int& number_of_buttons);

    // getters 
    const std::vector<Button>& get_buttons() const;
    const Button& get_button(const int& index) const;
    int get_number_of_buttons() const;

    void reset_buttons_state(); // reset the state of all the buttons to UNACTIVE
    void change_button_state(const int& index, const int& new_state); // change the state of a button 
};



/////////////////////////////////////////////////////////////////////////////////////
// helper functions to manage the graphic rendering
/////////////////////////////////////////////////////////////////////////////////////
// functions to apply a blur effect to the renderer
void create_gaussian_kernel(std::vector<double>& kernel, const int& radius);
void apply_horizontal_blur(std::vector<Uint32>& pixels, std::vector<Uint32>& temp_pixels, const int& width, const int& height, const std::vector<double>& kernel, const int& radius);
void apply_vertical_blur(std::vector<Uint32>& temp_pixels, std::vector<Uint32>& pixels, const int& width, const int& height, const std::vector<double>& kernel, const int& radius);
void apply_blur(SDL_Renderer* renderer, const int& width, const int& height, const int& radius);

// function to add an image to the render
void add_image_to_render(const std::string& filename, SDL_Renderer* renderer, const SDL_Rect& dest_rect);

// function to add a text to the render
void add_font_to_render(SDL_Renderer* renderer, const std::string& text, const int& font_size, const SDL_Color& text_color, const int& width_of_middle_of_text, const int& heigth_of_middle_of_text);

// function to draw a rectangle with a border where we usually click on a the screen
void draw_the_boundary_rect(SDL_Renderer* renderer, const SDL_Rect& boundary_move_rect, const SDL_Color& color);

// function to draw a filled circle
void draw_filled_circle(SDL_Renderer *renderer, const SDL_Color& color, const int& center_col, const int& center_row, const int& radius);

// function to check if a point is in a rectangle
bool is_point_in_rect(const int& x, const int& y, const SDL_Rect& rect);
    

#endif // GRAPHIC_HPP