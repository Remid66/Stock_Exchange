#include "graphic.hpp"


// constructor
Graphic_Buttons::Graphic_Buttons(const int& number_of_buttons) : Number_of_Buttons(number_of_buttons)
{
    Buttons.resize(Number_of_Buttons);

    // we initialize the buttons as unactive
    for (int i = 0; i < Number_of_Buttons; i++){
        Buttons[i].state = UNACTIVE;
    }

    // we then place them 

/*
to do 
*/
}

// getters
const std::vector<Button>& Graphic_Buttons::get_buttons() const
{
    return Buttons;
}

const Button& Graphic_Buttons::get_button(const int& index) const
{
    return Buttons[index];
}

int Graphic_Buttons::get_number_of_buttons() const
{
    return Number_of_Buttons;
}

// reset the state of all the buttons to UNACTIVE
void Graphic_Buttons::reset_buttons_state()
{
    for (int i = 0; i < Number_of_Buttons; i++){
        Buttons[i].state = UNACTIVE;
    }
}

// change the state of a button
void Graphic_Buttons::change_button_state(const int& index, const int& new_state)
{
    Buttons[index].state = new_state;
}



// functions to apply a blur effect to the renderer
void create_gaussian_kernel(std::vector<float>& kernel, const int& radius)
{   
    kernel.resize(2*radius+1);
    const float& sigma = radius/2.0f;
    const float& sigma_sq = 2.0f*sigma*sigma;
    float sum = 0.0f;

    for (int i = -radius; i <= radius; ++i){
        kernel[i + radius] = exp(-(i * i) / sigma_sq);
        sum += kernel[i + radius];
    }

    // normalize the kernel
    for (int i = 0; i < 2*radius+1; ++i){
        kernel[i] /= sum;
    }
}

void apply_horizontal_blur(std::vector<Uint32>& pixels, std::vector<Uint32>& temp_pixels, const int& width, const int& height, const std::vector<float>& kernel, const int& radius)
{
    for (int y = 0; y < height; ++y){
        for (int x = 0; x < width; ++x){
            float r = 0, g = 0, b = 0, a = 0;

            for (int k = -radius; k <= radius; ++k){
                const int& nx = x + k;
                if (nx >= 0 && nx < width){
                    Uint32 pixel = pixels[y * width + nx];
                    SDL_Color color;
                    SDL_GetRGBA(pixel, SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32), &color.r, &color.g, &color.b, &color.a);

                    const float& weight = kernel[k + radius];

                    r += color.r * weight;
                    g += color.g * weight;
                    b += color.b * weight;
                    a += color.a * weight;
                }
            }

            temp_pixels[y * width + x] = SDL_MapRGBA(SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32), (Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a);
        }
    }
}

void apply_vertical_blur(std::vector<Uint32>& temp_pixels, std::vector<Uint32>& pixels, const int& width, const int& height, const std::vector<float>& kernel, const int& radius)
{    
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            float r = 0, g = 0, b = 0, a = 0;

            for (int k = -radius; k <= radius; ++k) {
                const int& ny = y + k;
                if (ny >= 0 && ny < height) {
                    Uint32 pixel = temp_pixels[ny * width + x];
                    SDL_Color color;
                    SDL_GetRGBA(pixel, SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32), &color.r, &color.g, &color.b, &color.a);

                    const float& weight = kernel[k + radius];

                    r += color.r * weight;
                    g += color.g * weight;
                    b += color.b * weight;
                    a += color.a * weight;
                }
            }

            pixels[y * width + x] = SDL_MapRGBA(SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32), (Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a);
        }
    }
}

void apply_blur(SDL_Renderer* renderer, const int& width, const int& height, const int& radius)
{
    SDL_Texture* blurred_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, width, height);
    if (blurred_texture == NULL){
        printf("blurred_texture creation failed: %s\n", SDL_GetError());
        return;
    }
    SDL_SetRenderTarget(renderer, blurred_texture);
    SDL_RenderCopy(renderer, blurred_texture, NULL, NULL);
    SDL_SetRenderTarget(renderer, NULL);

    // create Gaussian kernel
    std::vector<float> kernel;
    create_gaussian_kernel(kernel, radius);

    // allocate pixel arrays
    std::vector<Uint32> pixels;
    std::vector<Uint32> temp_pixels;
    // read the current pixels from the renderer
    SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_RGBA32, pixels.data(), width * sizeof(Uint32));

    // apply horizontal and vertical blur
    apply_horizontal_blur(pixels, temp_pixels, width, height, kernel, radius);
    apply_vertical_blur(temp_pixels, pixels, width, height, kernel, radius);

    // update the texture with blurred pixels
    SDL_UpdateTexture(blurred_texture, NULL, pixels.data(), width * sizeof(Uint32));

    SDL_RenderCopy(renderer, blurred_texture, NULL, NULL);
    SDL_DestroyTexture(blurred_texture);
}


// function to add an image to the render
void add_image_to_render(const std::string& filename, SDL_Renderer* renderer, const SDL_Rect& dest_rect)
{
    // load the BMP image
    auto surface_deleter = [](SDL_Surface* surface) {
        if (surface) SDL_FreeSurface(surface);
    };
    std::unique_ptr<SDL_Surface, decltype(surface_deleter)> image(IMG_Load(filename.c_str()), surface_deleter);
    if (!image) {
        std::cerr << "Image loading failed: " << IMG_GetError() << std::endl;
        return; // Exit if the image failed to load.
    }
    
    // create texture from surface
    auto texture_deleter = [](SDL_Texture* texture) {
        if (texture) SDL_DestroyTexture(texture);
    };
    std::unique_ptr<SDL_Texture, decltype(texture_deleter)> texture(SDL_CreateTextureFromSurface(renderer, image.get()), texture_deleter);
    if (!texture) {
        std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
        return; // Exit if the texture creation failed.
    }
    
    // copy the texture to the renderer using the destination rectangle.
    SDL_RenderCopy(renderer, texture.get(), nullptr, &dest_rect);
}


// function to add a text to the render
void add_font_to_render(SDL_Renderer* renderer, const std::string& text, const int& font_size, SDL_Color text_color, const int& width_of_middle_of_text, const int& heigth_of_middle_of_text)
{
    // load font
    TTF_Font* font = TTF_OpenFont("/Library/Fonts/Arial Unicode.ttf", font_size);
    if (font == NULL){
        printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        return;
    }

    // create text surface and texture
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), text_color);
    if (textSurface == NULL){
        printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
        TTF_CloseFont(font);
        return;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == NULL){
        printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
        SDL_FreeSurface(textSurface);
        TTF_CloseFont(font);
        return;
    }
    SDL_FreeSurface(textSurface);

    // add the text to the render at the given position
    int textWidth, textHeight;
    SDL_QueryTexture(textTexture, NULL, NULL, &textWidth, &textHeight);
    SDL_Rect textRect = {width_of_middle_of_text-textWidth/2, heigth_of_middle_of_text-textHeight/2, textWidth, textHeight};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

    SDL_DestroyTexture(textTexture);
    TTF_CloseFont(font);
}


// draw a rectangle with a border where we usually click on a the screen
void draw_the_boundary_rect(SDL_Renderer* renderer, const SDL_Rect& boundary_move_rect, const SDL_Color& color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer, &boundary_move_rect); 
    // we draw the line around the piece 
    // we wanted to make it thicker so here is a way to do it, don't know if it's the best way
    // Draw top line
    for (int i = 0; i < 5; ++i){
        SDL_RenderDrawLine(renderer, boundary_move_rect.x, boundary_move_rect.y + i, boundary_move_rect.x + boundary_move_rect.w, boundary_move_rect.y + i);
    }
    // Draw right line
    for (int i = 0; i < 5; ++i){
        SDL_RenderDrawLine(renderer, boundary_move_rect.x + boundary_move_rect.w - i, boundary_move_rect.y, boundary_move_rect.x + boundary_move_rect.w - i, boundary_move_rect.y + boundary_move_rect.h);
    }
    // Draw bottom line
    for (int i = 0; i < 5; ++i){
        SDL_RenderDrawLine(renderer, boundary_move_rect.x, boundary_move_rect.y + boundary_move_rect.h - i, boundary_move_rect.x + boundary_move_rect.w, boundary_move_rect.y + boundary_move_rect.h - i);
    }
    // Draw left line
    for (int i = 0; i < 5; ++i){
        SDL_RenderDrawLine(renderer, boundary_move_rect.x + i, boundary_move_rect.y, boundary_move_rect.x + i, boundary_move_rect.y + boundary_move_rect.h);
    }

}


// function to draw a filled circle
void draw_filled_circle(SDL_Renderer *renderer, const SDL_Color& color, const int& center_col, const int& center_row, const int& radius)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    int x, y, dl;
    for (y = -radius; y <= radius; y++){
        dl = (int)sqrt(radius * radius - y * y);
        for (x = -dl; x <= dl; x++){
            SDL_RenderDrawPoint(renderer, center_col + x, center_row + y);
        }
    }

}


// function to check if a point is in a rectangle
bool is_point_in_rect(const int& x, const int& y, const SDL_Rect& rect)
{
    return (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h);
}

