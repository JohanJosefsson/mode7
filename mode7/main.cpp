#define _CRT_SECURE_NO_WARNINGS
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <list>
#define _USE_MATH_DEFINES 
#include <math.h>
#include <cassert>
#include <array>

#define JJ_FULLSCREEN

#define maxTimePerFrame (sf::seconds(1.f / (80.0)))
// TODO should be minTime? There should be a max too...

/* Once created, this class returns the parameter values necessary for creating a main window */
class Screen
{
	static int width_;
	static int height_;
	sf::VideoMode videoMode_;
	sf::String title_;
	int style_;
public:
	int getWidth() const { return width_; };
	int getHeight() const { return height_; };
	sf::VideoMode getVideoMode() const { return videoMode_; };
	sf::String getTitle() const { return title_; };
	int getStyle() const { return style_; };
	Screen() {
#ifdef JJ_FULLSCREEN
		// Display the list of all the video modes available for fullscreen
		std::vector<sf::VideoMode> modes = sf::VideoMode::getFullscreenModes();
		for (std::size_t i = 0; i < modes.size(); ++i)
		{
			sf::VideoMode mode = modes[i];
			std::cout << "Mode #" << i << ": "
				<< mode.width << "x" << mode.height << " - "
				<< mode.bitsPerPixel << " bpp" << std::endl;
		}

		sf::VideoMode mode = modes[0];
		// Create a window with the same pixel depth as the desktop
		sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
		videoMode_ = sf::VideoMode(mode.width, mode.height, desktop.bitsPerPixel);
		title_ = sf::String("Full screen");
		style_ = sf::Style::Fullscreen;
		const int screen_w = mode.width;
		const int screen_h = mode.height;
#else
		videoMode_ = sf::VideoMode(700, 400);
		title_ = sf::String("Oh yeah!");
		style_ = sf::Style::Default;
		const int screen_w = 700;
		const int screen_h = 400;
#endif
		width_ = screen_w;
		height_ = screen_h;
	}
};
int Screen::width_ = 0;
int Screen::height_ = 0;





/* Encapsulates calls to the SFML joystick */
class JoyS {
	int get_js()
	{
		int js = 0;
		sf::Joystick::update();
		while (js < MAX_JS + 1)
		{
			if (sf::Joystick::isConnected(js)) {
				break;
			}
			js++;
		}
		if (js > MAX_JS) {
		}
		return js;
	}
	int js;
	float jsx;
	static const int MAX_JS = 7;
	bool is_connected;

public:
	JoyS() : is_connected(false), jsx(0.0) {};
	bool connect()
	{
		js = get_js();
		if (js > MAX_JS) {
			// No joystick
			is_connected = false;
		}
		else {
			is_connected = true;
		}
		return is_connected;
	}
#if 0
	static void reconnect()
	{

		if (js > MAX_JS || !sf::Joystick::isConnected(js)) {
			js = get_js();
		}
	}
#endif
	float get()
	{
		if (is_connected)
		{
			jsx = sf::Joystick::getAxisPosition(js, sf::Joystick::X);
		}
		return jsx;
	}
#if 0
	void fullLeft()
	{
		jsx = 15.0;// JS_MAX;
	}
	void fullRight()
	{
		jsx = -15.0;// JS_MIN;
	}
	void straightDown()
	{
		jsx = 0.0;
	}
#endif
};

class Text
{
	sf::Font font_;
	sf::Text text_;
	static const int SZ_BUF = 200;
	char textBuf_[SZ_BUF];
	void init()
	{

		//if (!font_.loadFromFile("C:\\Windows\\fonts\\arial.ttf"))
		if (!font_.loadFromFile("C:\\Windows\\fonts\\DejaVuSansMono.ttf"))
		{
			printf("Error loading font.\n");
			exit(1);
		}
		// select the font
		text_.setFont(font_);
		text_.setCharacterSize(24 * 4); // in pixels, not points!
		text_.setFillColor(sf::Color::Cyan);
		text_.setOutlineColor(sf::Color::Blue);
		text_.setOutlineThickness(0.7);
		textBuf_[0] = '\0';
	}

public:
	Text()
	{
		init();
		//text_.setPosition(0.0, 0.0);
	}
	Text(int pos)
	{
		init();
		text_.setPosition(0.0, pos);
	}
	~Text() {};
	char * getBuf() { return textBuf_; }

	void draw(sf::RenderWindow& bkg)
	{
		text_.setString(textBuf_);
		bkg.draw(text_);
	}
};



/* Keeps info about the physical board (joystick) */
class Board
{
	float js_max;
	float js_min;
	static Board * instance_;
	JoyS joyS;
	Text text;
	Text oText;
	bool observability;
	bool is_connected;
	const static int N = 20; // filter depth
	std::array<float, N> a; // filter for calibration
	int ai; // index within the array a
	void print()
	{
		std::cout << "MAX: " << js_max << " MIN: " << js_min << std::endl;
	}

	float toNorm(float js) const
	{
		return 2.0 * js / (js_max - js_min);
	};

	sf::Time calibration_time;
	bool is_calibrating;
	float ncval;
	float norm;
	float js;

	int writetofile(float min, float max)
	{
		int num;
		FILE *fptr;
		fptr = fopen("calibration.txt", "w");

		if (fptr == NULL)
		{
			printf("Error!");
			exit(1);
		}

		fprintf(fptr, "%f %f", min, max);
		fclose(fptr);

		return 0;
	}

	bool readfromfile(float& min, float& max)
	{
		int num;
		FILE *fptr;

		if ((fptr = fopen("calibration.txt", "r")) == NULL) {
			printf("Could not open file\n");

			// Program exits if the file pointer returns NULL.
			// exit(1);
			return false;
		}

		fscanf(fptr, "%f %f", &min, &max);

		printf("Value (from file) of min,max=%f %f\n", min, max);
		fclose(fptr);

		return true;
	}




public:

	Board() : js_max(13), js_min(-15), is_calibrating(false), ncval(0.0), norm(0.0), text(200), oText(100), observability(false)
	{
		is_connected = joyS.connect();
		float f1, f2;
		if (readfromfile(f1, f2))
		{
			js_min = f1;
			js_max = f2;
		}

	};


	float get()
	{

		float ret;
		if (is_connected)
		{
			js = joyS.get();
			norm = toNorm(js);
			if (norm > 1.0)norm = 1.0;
			if (norm < -1.0)norm = -1.0;
			ret = norm;
		}
		else {
			ret = ncval;
		}
		return ret;
	}
	float getNorm() const { return norm; };
	void set(float val)
	{
		ncval = val;
	}
	void setObservability(bool obs)
	{
		observability = obs;
	}
	void tick(sf::Time dTime)
	{

		if (observability)
		{
			sprintf(oText.getBuf(), "% 4.2f % 4.2f % 4.2f % 4.2f", js_min, js_max, js, norm);
		}
		else {
			sprintf(oText.getBuf(), "");
		}
		if (!is_calibrating) return;

		// Calibrating...

// calibrating with filter
		float j = joyS.get();
		a[ai] = j;
		ai++;
		ai %= N;
		float sum = 0.0;
		for (int i = 0; i < N; i++)
		{
			float e = a[((ai - i) + N) % N];
			sum += e;
		}
		float f = sum / N;

		std::cout << f << std::endl;
		if (f > js_max) { js_max = f; std::cout << "          max" << std::endl; };
		if (f < js_min) { js_min = f;  std::cout << "     min" << std::endl; };
		//print();
		sprintf(text.getBuf(), "Calibrating... ");

		// Time to end calibration?
		calibration_time += dTime;
		if (calibration_time > sf::seconds(15))
		{
			is_calibrating = false;
			sprintf(text.getBuf(), "");
			writetofile(js_min, js_max);
		}

	}
	void draw(sf::RenderWindow& bkg)
	{
		text.draw(bkg);
		oText.draw(bkg);
	}
	void startCalibration()
	{
		calibration_time = sf::Time::Zero;
		is_calibrating = true;
		js_max = 1.0;
		js_min = -1.0;
	}

};

/* Demo thing that let a ball float over the screen */
class Ball
{
	float x;
	float y;
	sf::CircleShape circle;
public:
	Ball() : x(100.0), circle(50.0) {};
	void move(float playerFwd, float playerSidews)
	{
		y += playerFwd;
		x -= playerSidews;
		if (y > 1000.0) y = 0.0;
		circle.setPosition(x, y);
	}
	void draw(sf::RenderWindow& bkg) {

		bkg.draw(circle);
	}
} Ball;




class Track
{
	sf::Image track_;
	sf::Image projection_;
	sf::Texture texture_;
	sf::Sprite sprite_;

	//static const int X = 20000;
	//static const int Y = 10000;
	int w_;
	int h_;



	sf::Color get2DTexture(int sx, int sy)
	{
		int tw = track_.getSize().x;
		int th = track_.getSize().y;
		int tx = sx + tw / 2;
		int ty = sy + th / 2;
		if (tx < 0 || ty < 0 || tx > tw || ty > th)return(sf::Color::Cyan);
		return track_.getPixel(tx ,ty);
	}

	// (0,0) in middle of screen, y increases upwards
	void putColorOnScreen(int x, int y, sf::Color c)
	{
		projection_.setPixel(x + w_/2 , h_/2 - y -1, c);
		//projection_.setPixel(0, 0, sf::Color::Cyan);
	}


	// (0,0) in middle of track, y increases upwards
	void putColorOnTrack(int sx, int sy, sf::Color c)
	{
//		projection_.setPixel(x + track_.getSize().x / 2, track_.getSize().x / 2 - y - 1, c);
		//projection_.setPixel(0, 0, sf::Color::Cyan);

	//	std::cout << "putColorOnTrack() " << sx << " " << sy << std::endl;

		int tw = track_.getSize().x;
		int th = track_.getSize().y;
		int tx = sx + tw / 2;
		int ty = sy + th / 2;
		if (tx < 0 || ty < 0 || tx > tw || ty > th) {
			std::cout << "." << std::endl;
			return;
		}
		track_.setPixel(tx, ty, c);






	}



public:
	Track(int w, int h)
	{
		w_ = w;
		h_ = h;
#if 0
		if (!track_.loadFromFile("../res/track.png"))
		{
			printf("Error loading pic!\n");
			exit(1);
		}
#else
		track_.create(w, h, sf::Color::Yellow);
		for (int x = 0; x < w_; x++)
		{
			if ((x / 20) % 2)
			{
				for (int y = 0; y < h_; y++)
				{
					track_.setPixel(x, y, sf::Color::Blue);
				}
			}
		}
		for (int y = 0; y < h_; y++)
		{
			if ((y / 20) % 2)
			{
				for (int x = 0; x < w_; x++)
				{
					track_.setPixel(x, y, sf::Color::Red);
				}
			}
		}

#endif
		projection_.create(w_, h_, sf::Color::Magenta);
	}

	void getTrackCoords(int xi, int yi, float& sx, float&sy)
	{
		float x = xi;
		float y = yi;
//		float sx, sy;
		float horizon, fov, px, py, pz;


		horizon = 2000;//20; //adjust if needed
		fov = 2000;// 200;

		px = x;
		py = fov;
		pz = y + horizon;

		//projection 
		sx = px / pz;
		sy = py / pz;

		//sxi = sx;
		//syi = sy;

	}
	void update()
	{
//#if 1
		// https://gamedev.stackexchange.com/questions/24957/doing-an-snes-mode-7-affine-transform-effect-in-pygame


		int yres = h_;
		int xres = w_;
		int y, x, horizon, fov, px, py, pz,  scaling;
		sf::Color color;
		float sx, sy;

/*
		putColorOnScreen(0, 0, sf::Color::White);
		putColorOnScreen(-1 * w_ / 2 + 1, h_ / 2 - 1, sf::Color::White);
		putColorOnScreen(w_ / 2 - 1, h_ / 2 - 1, sf::Color::White);
		putColorOnScreen(w_ / 2 - 1, -h_ / 2 + 1, sf::Color::White);
		*/
		std::cout << ":" << std::endl;
		getTrackCoords(0, 0, sx, sy);
		std::cout << sx << " " << sy << std::endl;
		getTrackCoords(-1 * w_ / 2 + 1, h_ / 2 - 1, sx, sy);
		std::cout << sx << " " << sy << std::endl;
		getTrackCoords(w_ / 2 - 1, h_ / 2 - 1, sx, sy);
		std::cout << sx << " " << sy << std::endl;
		getTrackCoords(w_ / 2 - 1, -h_ / 2 + 1, sx, sy);
		std::cout << sx << " " << sy << std::endl;

		for (y = -yres / 2; y < yres / 2; y++)
			for (x = -xres / 2; x < xres / 2; x++)
			{
				/*
				horizon = 5000;//20; //adjust if needed
				fov = 2000;// 200;
				
				px = x;
				py = fov;
				pz = y + horizon;

				//projection 
				sx = px / pz;
				sy = py / pz;

				*/

				getTrackCoords(x, y, sx, sy);
				scaling = 200; //adjust if needed, depends of texture size


				color = get2DTexture(sx * scaling, sy * scaling);
				//putColorOnTrack(sx * scaling, sy * scaling, sf::Color::White);


				//put (color) at (x, y) on screen
				putColorOnScreen(x, y, color);

			}


//		putColorOnTrack(0.0, 0.0, sf::Color::White);

		// TODO random +1,-1 to make it work...?
		putColorOnScreen(0, 0, sf::Color::White);
		putColorOnScreen(-1*w_/2 +1, h_/2-1, sf::Color::White);
		putColorOnScreen( w_ / 2 -1, h_ / 2 - 1, sf::Color::White);
		putColorOnScreen(w_ / 2 - 1, -h_ / 2 + 1, sf::Color::White);

//#else
		//projection_ = track_;


//#endif







		texture_.loadFromImage(projection_);
		sprite_.setTexture(texture_);
	}
	void draw(sf::RenderWindow& bkg)
	{
		bkg.draw(sprite_);
	}


};

int main()
{
	Board board;

	float jsx = 0.0; //TODO

	Screen screen;
	sf::RenderWindow window(screen.getVideoMode(), screen.getTitle(), screen.getStyle());

	window.setMouseCursorVisible(false);
	window.setVerticalSyncEnabled(true);



	sf::Clock clock; // starts the clock
	sf::Time timeSinceLastUpdate = sf::Time::Zero;

	Text text;
	Track track(screen.getWidth(), screen.getHeight());

	//Tree tree(screen.getWidth(), screen.getHeight(), ct);

	// run the program as long as the window is open
	while (window.isOpen())
	{
		// check all the window's events that were triggered since the last iteration of the loop
		sf::Event event;
		while (window.pollEvent(event))
		{
			// "close requested" event: we close the window
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::KeyPressed)
			{
				switch (event.key.code) {
				case sf::Keyboard::Escape:
					window.close();
					break;
				}
			}
		}
		//window.clear(graphicData.getBkgCol());


		// draw everything here...
		// window.draw(...);

		//Ball.draw(window);

		board.draw(window);
		//tree.draw(window);
		track.draw(window);

		// end the current frame
		window.display();

		// Check the clock and update if time is due
		//
		timeSinceLastUpdate += clock.restart();
		while (timeSinceLastUpdate > maxTimePerFrame)
		{
			sf::Time dTime = timeSinceLastUpdate;
			timeSinceLastUpdate = sf::Time().Zero;

			// Update stuff...


			board.tick(dTime);
			track.update();
			//tree.move(rfwd, rsw);
		}
		jsx = board.get();
		int printjsx = (int)jsx;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		{
			//
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
			board.set(0.0);
#if 0
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
			{
				std::cout << "UP" << std::endl;
			}
			else {
				std::cout << "up" << std::endl;
			}
#endif
		}


		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::O)) {
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
			{
				board.setObservability(false);
			}
			else {
				board.setObservability(true);
			}
		}

		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
			board.set(0.0);
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
			{
				board.set(-0.99);
			}
			else {
				board.set(-0.7);
			}
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
			{
				board.set(0.99);
			}
			else {
				board.set(0.7);
			}
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
			board.startCalibration();
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
		}

	}
	return 0;
}
