#include "cub3d.h"

/*
** malloc_arrays: выделение памяти под масив спрайтов.
*/

static void malloc_arrays(t_cub *cub)
{
    cub->x = malloc(sizeof(float) * cub->p.coll_sprite);
    cub->y = malloc(sizeof(float) * cub->p.coll_sprite);
}

/*
** save_position_sprites: сохранить позицию спрайтов
** и вернуть их количество.
*/

static int save_position_sprites(t_cub *cub)
{
    int i;
    int j;
    int coll;

    i = 0;
    coll = 0;
    while (cub->p.map[i] != NULL)
    {
        j = 0;
        while (cub->p.map[i][j] != '\0')
        {
            if (cub->p.map[i][j] == 'B')
            {
                cub->x[coll] = j + 0.5;
                cub->y[coll] = i + 0.5;
                coll++;
            }
            ++j;
        }
        ++i;
    }
    return (coll);
}

/*
** print_sprite: печать спрайта.
*/

static void print_sprite(t_cub *cub)
{
    int i;

    i = 0;
    while (i < cub->p.coll_sprite)
    {
        //перевести положение спрайта относительно камеры
        double spriteX = cub->x[i] - cub->plr.x;
        double spriteY = cub->y[i] - cub->plr.y;
    
        //требуется для правильного умножения матриц
        double invDet = 1.0 / (cub->plr.planeX * cub->plr.dirY - cub->plr.dirX * cub->plr.planeY);

        double transformX = invDet * (cub->plr.dirY * spriteX - cub->plr.dirX * spriteY);

        //это на самом деле глубина внутри экрана, то, что Z есть в 3D
        double transformY = invDet * (-cub->plr.planeY * spriteX + cub->plr.planeX * spriteY); 

        int spriteScreenX = (int)((cub->p.resolution_w / 2) * (1 + transformX / transformY));

        //вычислите высоту спрайта на экране
        int spriteHeight = abs((int)(cub->p.resolution_l / (transformY))); //с помощью transformY' вместо реального расстояния предотвращает рыбий глаз
        //вычислите самый низкий и самый высокий пиксель для заполнения текущей полосы
        int drawStartY = -spriteHeight / 2 + cub->p.resolution_l / 2;
        if (drawStartY < 0)
            drawStartY = 0;
        int drawEndY = spriteHeight / 2 + cub->p.resolution_l / 2;
        if (drawEndY >= cub->p.resolution_l)
            drawEndY = cub->p.resolution_l - 1;

        //вычислить ширину спрайта
        int spriteWidth = abs((int)(cub->p.resolution_l / (transformY)));
        int drawStartX = -spriteWidth / 2 + spriteScreenX;
        if (drawStartX < 0) drawStartX = 0;
            int drawEndX = spriteWidth / 2 + spriteScreenX;
        if (drawEndX >= cub->p.resolution_w)
            drawEndX = cub->p.resolution_w - 1;

        //петля через каждую вертикальную полосу спрайта на экране
        int stripe;
        int y;

        stripe = drawStartX;
        while (stripe < drawEndX)
        {
            int texX = (int)(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * TEXWIDTH / spriteWidth) / 256;
            if (transformY > 0 && stripe > 0 && stripe < cub->p.resolution_w)
            
            y = drawStartY;
            while (y < drawEndY) //для каждого пикселя текущей полосы
            {
                int d = (y) * 256 - cub->p.resolution_l * 128 + spriteHeight * 128;
                int texY = ((d * TEXHEIGHT) / spriteHeight) / 256;
                int color = get_pixel(&cub->texture_sprite, texX, texY);
                if (color != 0)
                    my_mlx_pixel_put(&cub->data, stripe, y, color);
                y++;
            }
            ++stripe;
        }
        ++i;
    }
}

/*
** print_map: печать карты.
*/

static  void  print_map(t_cub *cub)
{
    cub->data.img = mlx_new_image(cub->mlx, cub->p.resolution_w, cub->p.resolution_l);
    cub->data.addr = mlx_get_data_addr(cub->data.img, &cub->data.bits_per_pixel, &cub->data.line_length, &cub->data.endian);

    int x = 0;
    while (x++ < cub->p.resolution_w)
    {
        //вычислить положение и направление луча
        double cameraX = 2 * x / (double)cub->p.resolution_w - 1; //x-coordinate in camera space
        double rayDirX = cub->plr.dirY + cub->plr.planeY * cameraX;
        double rayDirY = cub->plr.dirX + cub->plr.planeX * cameraX;

        //в каком квадрате карты мы находимся
        int mapX = (int)(cub->plr.y);
        int mapY = (int)(cub->plr.x);

        //длина луча от текущей позиции до следующей стороны x или y
        double sideDistX;
        double sideDistY;

        //длина луча от одной стороны x или y до следующей стороны x или y
        double deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX));
        double deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY));
        double perpWallDist;

        //в каком направлении делать шаг в направлении x или y (либо +1, либо -1)
        int stepX;
        int stepY;

        int hit = 0; //был ли удар об стену?
        int side; //была ли поражена стена NS или WE?

        //вычислить шаг и начальную сторону sideDist
        if(rayDirX < 0)
        {
            stepX = -1;
            sideDistX = (cub->plr.y - mapX) * deltaDistX;
        }
        else
        {
            stepX = 1;
            sideDistX = (mapX + 1.0 - cub->plr.y) * deltaDistX;
        }
        if(rayDirY < 0)
        {
            stepY = -1;
            sideDistY = (cub->plr.x - mapY) * deltaDistY;
        }
        else
        {
            stepY = 1;
            sideDistY = (mapY + 1.0 - cub->plr.x) * deltaDistY;
        }
        //проанализировать DDA
        while (hit == 0)
        {
            //переход к следующему квадрату карты, ИЛИ в направлении x, ИЛИ в направлении y
            if(sideDistX < sideDistY)
            {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            }
            else
            {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }
            //Проверьте, не ударился ли луч об стену
            if(cub->p.map[mapX][mapY] == '1')
                hit = 1;
        }
        //Вычислите расстояние, проецируемое на направление камеры (Евклидово расстояние даст эффект рыбьего глаза)
        if (side == 0)
            perpWallDist = (mapX - cub->plr.y + (1 - stepX) / 2) / rayDirX;
        else
            perpWallDist = (mapY - cub->plr.x + (1 - stepY) / 2) / rayDirY;

        //Вычислите высоту линии для рисования на экране
        int lineHeight = (int)(cub->p.resolution_l / perpWallDist);

        //вычислите самый низкий и самый высокий пиксель для заполнения текущей полосы
        int drawStart = -lineHeight / 2 + cub->p.resolution_l / 2;
        if (drawStart < 0)
            drawStart = 0;

        int drawEnd = lineHeight / 2 + cub->p.resolution_l / 2;
        if (drawEnd >= cub->p.resolution_l)
            drawEnd = cub->p.resolution_l - 1;

        //вычислите значение wallxoven
        double wallX; //куда именно ударилась стена
        if (side == 0)
            wallX = cub->plr.x + perpWallDist * rayDirY;
        else
            wallX = cub->plr.y + perpWallDist * rayDirX;
        wallX -= floor((wallX));
    
        //координата x на текстуре
        int texX = (int)(wallX * (double)(TEXWIDTH));
        if (side == 0 && rayDirX > 0)
            texX = TEXWIDTH - texX - 1;
        if (side == 1 && rayDirY < 0)
            texX = TEXWIDTH - texX - 1;

        // Насколько увеличить координату текстуры на пиксель экрана
        double step = 1.0 * TEXHEIGHT / lineHeight;
        // Начальная координата текстуры
        double texPos = (drawStart - cub->p.resolution_l / 2 + lineHeight / 2) * step;  

        int y = 0;
        while (y++ < cub->p.resolution_l)
        {
            if (y < drawStart) //поталок
                my_mlx_pixel_put(&cub->data, x, y, create_rgb(cub->p.ceilling_r, cub->p.ceilling_g, cub->p.ceilling_b));
            if (y >= drawStart && y <= drawEnd) //стена
            {
                // Приведите координату текстуры к целому числу и замаскируйте ее с помощью (TEXHEIGHT - 1) в случае переполнения
                int texY = (int)texPos & (TEXHEIGHT - 1);
                texPos += step;
                if (side == 0) //N и S
                    if (stepX > 0) //S
                        my_mlx_pixel_put(&cub->data, x, y, get_pixel(&cub->texture_s, texX, texY));
                    else //N
                        my_mlx_pixel_put(&cub->data, x, y, get_pixel(&cub->texture_n, texX, texY));
                else //W и E
                    if (stepY > 0) //E
                        my_mlx_pixel_put(&cub->data, x, y, get_pixel(&cub->texture_e, texX, texY));
                    else //W
                        my_mlx_pixel_put(&cub->data, x, y, get_pixel(&cub->texture_w, texX, texY));
            }
            if (y > drawEnd && y < cub->p.resolution_l) //пол
                 my_mlx_pixel_put(&cub->data, x, y, create_rgb(cub->p.floore_r, cub->p.floore_g, cub->p.floore_b));
        }
    }
    print_sprite(cub);
    mlx_put_image_to_window(cub->mlx, cub->mlx_win, cub->data.img, 0, 0);
}

/*
** key_hook: взаимодействие с клавиатурой.
*/

static int key_hook(int keycode, t_cub *cub)
{
    mlx_destroy_image(cub->mlx, cub->data.img);

    //модификаторы скорости
    double moveSpeed = 0.1; //постоянное значение выражается в квадратах в секунду
    double rotSpeed = 0.1; //постоянное значение находится в радианах/секунде

    //движение вперед, если перед вами нет стены
    if(keycode == 13)
    {
        if(cub->p.map[(int)(cub->plr.y + cub->plr.dirY * moveSpeed)][(int)(cub->plr.x)] == '*')
            cub->plr.y += cub->plr.dirY * moveSpeed;
        if(cub->p.map[(int)(cub->plr.y)][(int)(cub->plr.x + cub->plr.dirX * moveSpeed)] == '*')
            cub->plr.x += cub->plr.dirX * moveSpeed;
    }
    //движение назад, если за вами нет стены
    if(keycode == 1)
    {
        if(cub->p.map[(int)(cub->plr.y - cub->plr.dirY * moveSpeed)][(int)(cub->plr.x)] == '*')
            cub->plr.y -= cub->plr.dirY * moveSpeed;
        if(cub->p.map[(int)(cub->plr.y)][(int)(cub->plr.x - cub->plr.dirX * moveSpeed)] == '*')
            cub->plr.x -= cub->plr.dirX * moveSpeed;
    }
     //движение влево
    if(keycode == 0)
    {
        if(cub->p.map[(int)(cub->plr.y - cub->plr.dirX * moveSpeed)][(int)(cub->plr.x)] == '*')
            cub->plr.y -= cub->plr.dirX * moveSpeed;
        if(cub->p.map[(int)(cub->plr.y)][(int)(cub->plr.x + cub->plr.dirY * moveSpeed)] == '*')
            cub->plr.x += cub->plr.dirY * moveSpeed;
    }
    //движение вправо
    if(keycode == 2)
    {
        if(cub->p.map[(int)(cub->plr.y + cub->plr.dirX * moveSpeed)][(int)(cub->plr.x)] == '*')
            cub->plr.y += cub->plr.dirX * moveSpeed;
        if(cub->p.map[(int)(cub->plr.y)][(int)(cub->plr.x - cub->plr.dirY * moveSpeed)] == '*')
            cub->plr.x -= cub->plr.dirY * moveSpeed;
    }
    //поворот вправо
    if(keycode == 124)
    {
        //как направление камеры, так и плоскость камеры должны быть повернуты
        double oldDirX = cub->plr.dirY;
        cub->plr.dirY = cub->plr.dirY * cos(-rotSpeed) - cub->plr.dirX * sin(-rotSpeed);
        cub->plr.dirX = oldDirX * sin(-rotSpeed) + cub->plr.dirX * cos(-rotSpeed);
        double oldPlaneX = cub->plr.planeY;
        cub->plr.planeY = cub->plr.planeY * cos(-rotSpeed) - cub->plr.planeX * sin(-rotSpeed);
        cub->plr.planeX = oldPlaneX * sin(-rotSpeed) + cub->plr.planeX * cos(-rotSpeed);
    }
    //поварот влево
    if(keycode == 123)
    {
        //как направление камеры, так и плоскость камеры должны быть повернуты
        double oldDirX = cub->plr.dirY;
        cub->plr.dirY = cub->plr.dirY * cos(rotSpeed) - cub->plr.dirX * sin(rotSpeed);
        cub->plr.dirX = oldDirX * sin(rotSpeed) + cub->plr.dirX * cos(rotSpeed);
        double oldPlaneX = cub->plr.planeY;
        cub->plr.planeY = cub->plr.planeY * cos(rotSpeed) - cub->plr.planeX * sin(rotSpeed);
        cub->plr.planeX = oldPlaneX * sin(rotSpeed) + cub->plr.planeX * cos(rotSpeed);
    }
    //закрытие окна на esc
    if (keycode == 53)
		close_win(cub);
    print_map(cub);
    printf("You put: %d\n", keycode);
    return (0);
}

/*
** start_cub3d: запуск окна, работа в 3D.
*/

void  start_cub3d(t_cub *cub)
{
    int width;
	int height;

    //начальный расположение игрока
    cub->plr.y = cub->p.playr_y;
    cub->plr.x = cub->p.playr_x;

    //количество спрайтов
    malloc_arrays(cub);
    cub->p.coll_sprite = save_position_sprites(cub);

    if (cub->plr.dir_symbol == 'N')
    {
        //начальный вектор направления
        cub->plr.dirY = -1; 
        cub->plr.dirX = 0;
        //плоскость камеры игрока
        cub->plr.planeY = 0;
        cub->plr.planeX = 0.66; //2d рейкастинг версия плоскости камеры
    }
    if (cub->plr.dir_symbol == 'S')
    {
        //начальный вектор направления
        cub->plr.dirY = 0.998295; 
        cub->plr.dirX = 0.058374;
        //плоскость камеры игрока
        cub->plr.planeY = 0.038527;
        cub->plr.planeX = -0.658875; //2d рейкастинг версия плоскости камеры
    }
    if (cub->plr.dir_symbol == 'W')
    {
        //начальный вектор направления
        cub->plr.dirY = 0.029199; 
        cub->plr.dirX = -0.999574;
        //плоскость камеры игрока
        cub->plr.planeY = -0.659719;
        cub->plr.planeX = -0.019272; //2d рейкастинг версия плоскости камеры
    }
    if (cub->plr.dir_symbol == 'E')
    {
        //начальный вектор направления
        cub->plr.dirY = 0.029200; 
        cub->plr.dirX = 0.999574;
        //плоскость камеры игрока
        cub->plr.planeY = 0.659719;
        cub->plr.planeX = -0.019272; //2d рейкастинг версия плоскости камеры
    }

    //установка последнего пикселя, что бы не былло Segmentation fault
    cub->p.resolution_l = cub->p.resolution_l - 1;

    cub->mlx = mlx_init();
    cub->mlx_win = mlx_new_window(cub->mlx, cub->p.resolution_w, cub->p.resolution_l, "cub3d");

    //схватываем текстуры N
    if (!(cub->texture_n.img = mlx_xpm_file_to_image(cub->mlx, cub->p.north_texture, &width, &height)))
        error("ERROR: No texture found for North", cub);
	cub->texture_n.addr = mlx_get_data_addr(cub->texture_n.img, &cub->texture_n.bits_per_pixel, &cub->texture_n.line_length, &cub->texture_n.endian);
    //схватываем текстуры S
    if (!(cub->texture_s.img = mlx_xpm_file_to_image(cub->mlx, cub->p.south_texture, &width, &height)))
        error("ERROR: No texture found for the South", cub);
	cub->texture_s.addr = mlx_get_data_addr(cub->texture_s.img, &cub->texture_s.bits_per_pixel, &cub->texture_s.line_length, &cub->texture_s.endian);
    //схватываем текстуры W
    if (!(cub->texture_w.img = mlx_xpm_file_to_image(cub->mlx, cub->p.west_texture, &width, &height)))
        error("ERROR: No texture found for the West", cub);
	cub->texture_w.addr = mlx_get_data_addr(cub->texture_w.img, &cub->texture_w.bits_per_pixel, &cub->texture_w.line_length, &cub->texture_w.endian);
    //схватываем текстуры E
    if (!(cub->texture_e.img = mlx_xpm_file_to_image(cub->mlx, cub->p.east_texture, &width, &height)))
        error("ERROR: No texture found for the East", cub);
	cub->texture_e.addr = mlx_get_data_addr(cub->texture_e.img, &cub->texture_e.bits_per_pixel, &cub->texture_e.line_length, &cub->texture_e.endian);
    //схватываем текстуры sprite
    if (!(cub->texture_sprite.img = mlx_xpm_file_to_image(cub->mlx, cub->p.sprite_texture, &width, &height)))
        error("ERROR: No texture found for the sprite", cub);
	cub->texture_sprite.addr = mlx_get_data_addr(cub->texture_sprite.img, &cub->texture_sprite.bits_per_pixel, &cub->texture_sprite.line_length, &cub->texture_sprite.endian);

    //печать карты
    print_map(cub);

    //музыка
    system("afplay ./sounds/C418-Door.mp3 & ");

    //управление
    mlx_hook(cub->mlx_win, 2, 1L<<0, key_hook, cub);
    mlx_hook(cub->mlx_win, 17, 1L<<0, close_win, cub);
    mlx_loop(cub->mlx);
}
