/* lmao */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "SDL2/SDL.h"

// The compliler needs to know that this function exists before it calls it, or something like that
int generate_terrain (int size, float scaling, float z_layer, float **height);

int main(){
    //
    // Init
    //

    printf("Starting\n");

    SDL_Init( SDL_INIT_VIDEO );
    SDL_Window *window = SDL_CreateWindow("planes but with less detail",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,1000,1000,SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    SDL_Renderer *renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    // Set pixel format to make the textures in
    Uint32 pixel_format_id = SDL_PIXELFORMAT_RGBA32; // A 32 bit format that lets the OS decide specifics
    SDL_PixelFormat *pixel_format = SDL_AllocFormat(pixel_format_id); // Get the actual format object from its ID
    
    //
    // Terrain Heights Array
    //

    int terrain_size = 100;

    // Make array for the terrain generator to fill (a texture i guess)
    // Allocating memory for the matrix which will store the altitudes
    // Allocate the first dimension as an array of float pointers
    float **height = malloc(sizeof(float*)*terrain_size);
    // Allocate each float pointer as an array of actual floats
    for (int i=0; i<terrain_size; i++) {
        height[i] = malloc(sizeof(float)*terrain_size);
    }

    generate_terrain(terrain_size, 4.0, .02, height); // Get a terrain height map

    //
    // Background
    //
    
    struct background_layer {
        int distance; // Distance from view
        double density; // Cloud thickness
        int shadow_offset[2];
        double position[2];
        double last_update_time[2]; // When each position was last updated
        SDL_Texture* texture; // Texture in video memory
        Uint32 *pixels; // Pixel data in normal memory
    };
    
    struct terrain_layer {
        int start_color[3];
        int end_color[3];
        float end_height;
        float start_height;
    };

    struct terrain_layer biome[] = {
        {
            .start_color = {36,36,85}, // Deep water
            .end_color = {36,109,170}, // Shallow water
            .start_height = -3, // Minimum value
            .end_height = 0, // Minimum value
        },{
            .start_color = {0,109,0}, // low land
            .end_color = {0,218,0}, // high land
            .start_height = 0, // Minimum value
            .end_height = 2, // halfway
        }
    };

    int terrain_layer_amount = sizeof(biome) / sizeof(struct terrain_layer);
    printf("There are %i terrain layer(s)\n",terrain_layer_amount);

    // Convert height map to pixel color map
    Uint32 land_pixels[terrain_size][terrain_size]; // Create the array to store the pixels
    for(int x=0; x < terrain_size; x++) {
        for(int y=0; y < terrain_size; y++) {
            for(int layer=0; layer < terrain_layer_amount; layer++) {
                if (biome[layer].start_height <= height[x][y] && biome[layer].end_height >= height[x][y]) { // the layer of interest
                    float pixel_color[3]; 
                    for(int color=0; color <= 2; color++) {
                        // adjust the height values to fit inside thingo
                        pixel_color[color] = biome[layer].start_color[color] + ((biome[layer].start_color[color] - biome[layer].end_color[color])*(height[x][y]-biome[layer].start_height))/(biome[layer].start_height - biome[layer].end_height);
                    }
                    land_pixels[x][y] = SDL_MapRGB(pixel_format,pixel_color[0],pixel_color[1],pixel_color[2]);
                }
            }
        }
    }
            
    // Put the land image into a texture
    SDL_Surface *land_surface = SDL_CreateRGBSurfaceWithFormatFrom(land_pixels, terrain_size, terrain_size, 0,terrain_size * sizeof(Uint32), pixel_format_id); // Through a surface 

    // Instantiate the land layer struct
    struct background_layer land = {
        .distance = 500,
        .texture = SDL_CreateTextureFromSurface(renderer,land_surface),
    };

    SDL_FreeSurface(land_surface);

    // Creates an array of cloud layers with their height and density already set, and the land already in the background
    struct background_layer background_layers[] = {land};
    
    int background_layer_amount = sizeof(background_layers) / sizeof(struct background_layer);
    printf("There are %i background layer(s)\n",background_layer_amount);

    // Convert height map to clouds
    for (int i = 1; i < background_layer_amount; i++) {
        // Run terrain generation
        generate_terrain(terrain_size, 1.5 ,1, height);        // Create a cloud pixel map from the height map provided
        Uint32 pixels[terrain_size][terrain_size];
        for(int xs=0; xs < terrain_size; xs++) {
            for(int y=0; y < terrain_size; y++) {
                if ((height)[xs][y] < background_layers[i].density) {
                    // Draw Transparent
                    pixels[xs][y] = SDL_MapRGBA(pixel_format, 0, 0, 0, 0);
                } else if (background_layers[i].density <= height[xs][y]){
                    // Draw Greyscale
                    int greyness = (1-(height[xs][y]-background_layers[i].density)/16) * 225 + 30;
                    pixels[xs][y] = SDL_MapRGBA(pixel_format, greyness, greyness, greyness, 255);
                }
            }
        }
        background_layers[i].pixels = *pixels; // Add the pixels to the struct for this layer
        SDL_Texture *cloud_complete_texture = SDL_CreateTexture(renderer, pixel_format_id, SDL_TEXTUREACCESS_TARGET, terrain_size, terrain_size);
        SDL_SetRenderTarget(renderer, cloud_complete_texture);
        SDL_Surface *cloud_surface = SDL_CreateRGBSurfaceWithFormatFrom(background_layers[i].pixels, terrain_size, terrain_size, 0,terrain_size * sizeof(Uint32), pixel_format_id);
        SDL_Texture *cloud_texture = SDL_CreateTextureFromSurface(renderer,cloud_surface);
        SDL_Rect cloud_dest = {0,0,terrain_size,terrain_size};
        // Create cloud shadows by blacking out the cloud texture
        SDL_Texture *cloud_shadow_texture = SDL_CreateTextureFromSurface(renderer,cloud_surface);
        SDL_SetTextureColorMod(cloud_shadow_texture, 30, 30, 30);
        SDL_SetTextureAlphaMod(cloud_shadow_texture, 140);
        SDL_Rect cloud_shadow_dest = {background_layers[i].shadow_offset[0],background_layers[i].shadow_offset[1],terrain_size,terrain_size};
        // Add the cloud shadows to the frame
        SDL_SetTextureBlendMode(cloud_complete_texture, SDL_BLENDMODE_BLEND);
        SDL_RenderCopyEx(renderer, cloud_shadow_texture, NULL, &cloud_shadow_dest, 90, NULL, SDL_FLIP_NONE);
        // Add the clouds to the frame
        SDL_RenderCopyEx(renderer, cloud_texture, NULL, &cloud_dest, 90, NULL, SDL_FLIP_NONE);
        // Something
        SDL_SetRenderTarget(renderer, NULL);
        background_layers[i].texture = cloud_complete_texture;
        SDL_FreeSurface(cloud_surface);
    }
    
    //
    // Sprites
    //
    
    // Avaliable: {"spitfire","me109","avro_lancaster","mosquito","dehavalland_vampire","horten_229","p38_lightening"}
    
    const char *sprite_names[] = {"spitfire"};
    
    int sprite_amount = sizeof(sprite_names) / sizeof(char *);
    
    struct sprite {
        SDL_Surface* surface;
        SDL_Texture* texture;
        double position[2];
        double velocity[2];
        double last_update_time[2];
        int size;
        char filename[];
    };
    
    struct sprite sprites[sprite_amount];
    
    // Load sprites
    for (int i = 0; i < sprite_amount; i++) {
        // Organise the filename extension
        strcpy(sprites[i].filename, sprite_names[i]);
        strcat(sprites[i].filename, ".bmp");
        // Load the sprite from the image
        sprites[i].surface = SDL_LoadBMP(sprites[i].filename);
        // Convert to the pixel format we're using
        SDL_ConvertSurface(sprites[i].surface, pixel_format, 0);
        // Create texture
        sprites[i].texture = SDL_CreateTextureFromSurface(renderer, sprites[i].surface);
        // Delete surface or something
        sprites[i].position[1] = 50;
        sprites[i].position[0] = 50;
        sprites[i].velocity[0] = 0;
        sprites[i].velocity[1] = 0;
    }

    //
    // Loop
    //
    
    // yep all these pixels ae the same size
    double pixel_scaling = 10;

    double view_velocity[] = {0,0};
    SDL_Event window_event;
    int window_h;
    int window_w;

    bool run = true;
    int velocity = 1;
    int left_speed = 0; 
    int right_speed = 0; 
    int up_speed = 0; 
    int down_speed = 0; 

    // Main loop that updates at vsync in case we ever need animations
    while (run) {
        while (SDL_PollEvent(&window_event)){
            switch( window_event.type ){
                case SDL_QUIT:
                    run = false;
                    break;
                    // Look for a keypress
                case SDL_KEYDOWN:
                    // Check the SDLKey values and move change the coords
                    switch( window_event.key.keysym.sym ){
                        case SDLK_LEFT:
                            left_speed = velocity;
                            break; 
                        case SDLK_RIGHT:
                            right_speed = velocity;
                            break; 
                        case SDLK_UP:
                            up_speed = velocity;
                            break; 
                        case SDLK_DOWN:
                            down_speed = velocity;
                            break; 
                        default:    
                            break;
                    }
                    break;
                case SDL_KEYUP:
                    // Check the SDLKey values and move change the coords
                    switch( window_event.key.keysym.sym ){
                        case SDLK_LEFT:
                            left_speed = 0;
                            break;
                        case SDLK_RIGHT:
                            right_speed = 0;
                            break;
                        case SDLK_UP:
                            up_speed = 0;
                            break;
                        case SDLK_DOWN:
                            down_speed = 0;
                            break;
                        default:    
                            break;
                    }
                    break;
            }
        }
        
        sprites[0].velocity[0] = right_speed - left_speed;
        sprites[0].velocity[1] = down_speed - up_speed; 

        //Stop you from getting speeding tickets on diagnols 
        if (sprites[0].velocity[0] != 0 && sprites[0].velocity[1] != 0){
            if ((sprites[0].velocity[0] == velocity || sprites[0].velocity[0] == -velocity ) && (sprites[0].velocity[1] == velocity || sprites[0].velocity[1] == -velocity)){
                sprites[0].velocity[0] = sprites[0].velocity[0]*0.5; 
                sprites[0].velocity[1] = sprites[0].velocity[1]*0.5;
            }   
        }

        //Stop you leaving the map 
        //if (sprites[0].position[0] <= 0){
        //    sprites[0].position[0] = 0; 
        //    if (sprites[0].velocity[0] < 0){
        //        sprites[0].velocity[0] = 0;
        //    }                  
        //}        

        // Draw each of the background layers with the correct position
        for (int i = 0; i < background_layer_amount; i++) {
            // Determine if it needs to be have its position changed
            for (int a = 0; a < 2 ; a++) {
                double velocity = view_velocity[a]/background_layers[i].distance;
                double time_since_update = SDL_GetTicks() - background_layers[i].last_update_time[a];
                background_layers[i].position[a] = background_layers[i].position[a] + velocity * time_since_update;
                background_layers[i].last_update_time[a] = SDL_GetTicks();
            }
            SDL_GetRendererOutputSize(renderer, &window_w, &window_h);
            SDL_Rect new_position = {background_layers[i].position[0],background_layers[i].position[1], terrain_size*pixel_scaling, terrain_size*pixel_scaling};
            // Add to frame
            SDL_RenderCopy(renderer,background_layers[i].texture,NULL,&new_position);
        }
        
        // Draw each of the sprites with the correct position
        for (int i = 0; i < sprite_amount; i++) {
            for (int a = 0; a < 2 ; a++) {
                double time_since_update = SDL_GetTicks() - sprites[i].last_update_time[a];
                sprites[i].position[a] = sprites[i].position[a] + sprites[i].velocity[a] * time_since_update;
                sprites[i].last_update_time[a] = SDL_GetTicks();
            }
            SDL_Rect dest = {sprites[i].position[0],sprites[i].position[1],sprites[i].surface->w*pixel_scaling,sprites[i].surface->h*pixel_scaling};
            SDL_RenderCopyEx(renderer, sprites[i].texture, NULL, &dest, 0, NULL, SDL_FLIP_NONE);
        }
        
        SDL_RenderPresent(renderer); // Show the completed frame and wait for vsync
        SDL_RenderClear(renderer); // Erase the screen (first action of the new frame)
    }
    
    //
    // Clean Up
    //
    free(height);    
    // Destroy things
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    //Quit SDL
    SDL_Quit();
    return 0;
}
