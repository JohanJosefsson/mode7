#define _CRT_SECURE_NO_WARNINGS
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <list>
#define _USE_MATH_DEFINES 
#include <math.h>
#include <cassert>
#include <array>
#include <vector>

//#define JJ_FULLSCREEN

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

	int w_;
	int h_;







	float wx = 0.5;
	float wy = 0.5;
	float theta = 0;
	float alpha = M_PI / 4;
	float near = 0.0005;
	float far = 0.015;


	void putColorOnTrack(float x, float y, sf::Color c)
	{
		if (x < 0.0 || x >= 1.0 || y < 0.0 || y >= 1.0)return;
		int tx = x * track_.getSize().x;
		int ty = y * track_.getSize().y;
		//std::cout << tx << " " << ty << std::endl;
		track_.setPixel(tx, ty, c);
	}


	sf::Color getColorSample(float x, float y)
	{
		if (x < 0.0 || x >= 1.0 || y < 0.0 || y >= 1.0)return sf::Color::Cyan;
		int tx = x * track_.getSize().x;
		int ty = y * track_.getSize().y;
		//std::cout << tx << " " << ty << std::endl;
		return track_.getPixel(tx, ty);
	}



	void plotOnTrack(float x, float y, sf::Color c)
	{

		int tx = x * track_.getSize().x;
		int ty = y * track_.getSize().y;

		for (int ox = -1; ox < 2; ox++)
		{
			for (int oy = -1; oy < 2; oy++)
			{
				int px = tx + ox;
				int py = ty + oy;
				if(px >= 0 && px < track_.getSize().x && py >= 0 && py < track_.getSize().y)
				track_.setPixel(px, py, c);
			}
		}
	}

	void updateSprite()
	{

		// far x left et c.
		float fx1 = wx + far * sinf(theta - alpha);
		float fx2 = wx + far * sinf(theta + alpha);
		float fy1 = wy - far * cosf(theta - alpha);
		float fy2 = wy - far * cosf(theta + alpha);

		float nx1 = wx + near * sinf(theta - alpha);
		float nx2 = wx + near * sinf(theta + alpha);
		float ny1 = wy - near * cosf(theta - alpha);
		float ny2 = wy - near * cosf(theta + alpha);


		if (paintTrack_) {
			plotOnTrack(fx1, fy1, sf::Color::Cyan);
			plotOnTrack(fx2, fy2, sf::Color::Blue);
			plotOnTrack(nx1, ny1, sf::Color::Magenta);
			plotOnTrack(nx2, ny2, sf::Color::Red);
		}

		for (int y = h_ - 1; y >= h_ / 2; y--)
		{
			float sampleDepth = (y - h_ / 2) / (h_ / 2.0);

			float startX = (fx1 - nx1) / sampleDepth + nx1;
			float startY = (fy1 - ny1) / sampleDepth + ny1;
			float endX = (fx2 - nx2) / sampleDepth + nx2;
			float endY = (fy2 - ny2) / sampleDepth + ny2;

			//putColorOnTrack(startX, startY, sf::Color::Green);
			//putColorOnTrack(endX, endY, sf::Color::Green);

			for (int x = 0; x < w_; x++)
			{
				float sampleWidth = (float)x / w_;
				float sx = (endX - startX)*sampleWidth + startX;
				float sy = (endY - startY)*sampleWidth + startY;


				//putColorOnTrack(sx, sy, sf::Color::White);
				sf::Color c = getColorSample(sx, sy);
				
				projection_.setPixel(x, y, c);
			}
		}

		plotOnTrack(wx, wy, sf::Color::White);


		if (showTrack_) {
			texture_.loadFromImage(track_);
		}
		else {
			texture_.loadFromImage(projection_);
		}

		sprite_.setTexture(texture_);
	}
public:
	// Debug flags
	bool paintTrack_ = false;
	bool showTrack_ = false;


	Track(int w, int h)
	{
		w_ = w;
		h_ = h;
#if 1
//		if (!track_.loadFromFile("../res/track.png"))
		if (!track_.loadFromFile("../res/omni.png"))
		{
			printf("Error loading pic!\n");
			exit(1);
		}
#else
		track_.create(w, h, sf::Color::Yellow);



#if 0
		for (int x = 0; x < w_; x++)
		{
			if ((x / 2) % 2)
			{
				for (int y = 0; y < h_; y++)
				{
					track_.setPixel(x, y, sf::Color::Blue);
				}
			}
		}
		for (int y = 0; y < h_; y++)
		{
			if (!((y / 2) % 2))
			{
				for (int x = 0; x < w_; x++)
				{
					if ((x / 2) % 2)track_.setPixel(x, y, sf::Color::Red);
				}
			}
		}

#else


		for (int x = 0; x < w_; x++)
		{
			for (int y = 0; y < h_; y++)
			{
				if ((x / 10) % 2 && (y / 10) % 2)track_.setPixel(x, y, sf::Color::Red);
			}
		}



#endif









#endif
		projection_.create(w_, h_, sf::Color::Magenta);
	}

	void tick(sf::Time dTime) {
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
			theta -= dTime.asSeconds()*(M_PI * 2) / (5.0);
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
			theta += dTime.asSeconds()*(M_PI * 2) / (5.0);
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
			wx += dTime.asSeconds()*(0.05)*sinf(theta);
			wy -= dTime.asSeconds()*(0.05)*cosf(theta);
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
			wx -= dTime.asSeconds()*(0.05)*sinf(theta);
			wy += dTime.asSeconds()*(0.05)*cosf(theta);
		}




	};




	void draw(sf::RenderWindow& bkg)
	{
		updateSprite();
		bkg.draw(sprite_);
	}


};



class D3Vec
{
	void clear()
	{
		for (int i = 0; i < 4; i++)
		{
			v[i] = 0.0;
		}
	}

public:

	D3Vec()
	{
		clear();
	}
	D3Vec(float x, float y, float z, float w)
	{
		v[0] = x;
		v[1] = y;
		v[2] = z;
		v[3] = w;
	}
	float v[4]; // x, y, z , w
	void print()
	{
		std::cout << "[ ";
		for (int r = 0; r < 4; r++)
		{
			std::cout << v[r];
			if(r < 2)std::cout << ", ";
			
		}
		std::cout << " ]";
		std::cout << std::endl;
	}
	static D3Vec pos(float x, float y, float z)
	{
		D3Vec r;
		r.v[0] = x;
		r.v[1] = y;
		r.v[2] = z;
		r.v[3] = 1.0;
		return r;
	}
};

class D3Mat
{
public:
	float v[4][4];  // [row][col]
	D3Mat()
	{
		for (int r = 0; r < 4; r++)
		{
			for (int c = 0; c < 4; c++)
			{
				v[r][c] = 0.0;
			}
		}

	}
	D3Vec cross(D3Vec t)
	{
		D3Vec d;
		for (int r = 0; r < 4; r++)
		{
			d.v[r] = 0.0;
			for (int c = 0; c < 4; c++)
			{
				d.v[r] += t.v[c] * v[r][c];
			}
		}
		return d;
	}
	static D3Mat unity()
	{
		D3Mat d;
		for (int i = 0; i < 4; i++)
		{
			d.v[i][i] = 1.0;
		}
		return d;
	}

	static D3Mat trans(D3Vec v)
	{
		D3Mat d = unity();
		for (int i = 0; i < 4; i++)
		{
			d.v[i][3] = v.v[i];
		}
		assert(d.v[3][3] == 1.0);
		return d;
	}



	static D3Mat scale(D3Vec v)
	{
		D3Mat d = unity();
		for (int i = 0; i < 4; i++)
		{
			d.v[i][i] = v.v[i];
		}
		d.v[3][3] = 1.0;
		return d;
	}





	// http://www.opengl-tutorial.org/assets/faq_quaternions/index.html#Q30

//	Q28.How do I generate a rotation matrix in the X - axis ?
//		------------------------------------------------------ -
//
//		Use the 4x4 matrix :
//
//	        |  1  0        0      0 |
//		M = |  0  cos(A) -sin(A)  0 |
//		    |  0  sin(A)  cos(A)  0 |
//		    |  0  0       0       1 |

	static D3Mat rotX(float a)
	{
		//row, col
		D3Mat d = unity();
		d.v[1][1] = cosf(a);
		d.v[1][2] = -sinf(a);
		d.v[2][1] = sinf(a);
		d.v[2][2] = cosf(a);
		return d;
	}



//		Q29.How do I generate a rotation matrix in the Y - axis ?
//		------------------------------------------------------ -

//		Use the 4x4 matrix :

//	        | cos(A)   0   sin(A)  0 |
//		M = | 0        1   0       0 |
//		    | -sin(A)  0   cos(A)  0 |
//		    |  0       0   0       1 |

	static D3Mat rotY(float a)
	{
		//row, col
		D3Mat d = unity();
		d.v[0][0] = cosf(a);
		d.v[0][2] = sinf(a);
		d.v[2][0] = -sinf(a);
		d.v[2][2] = cosf(a);
		return d;
	}



//		Q30.How do I generate a rotation matrix in the Z - axis ?
//		------------------------------------------------------ -

//		Use the 4x4 matrix :

//	        | cos(A) - sin(A)    0   0 |
//		M = | sin(A)   cos(A)    0   0 |
//		    |  0        0        1   0 |
//		    |  0        0        0   1 |

	static D3Mat rotZ(float a)
	{
		D3Mat d = unity();
		d.v[0][0] = cosf(a);
		d.v[0][1] = sinf(a);
		d.v[1][0] = -sinf(a);
		d.v[1][1] = cosf(a);		
		return d;
	}





	void print()
	{
		for (int r = 0; r < 4; r++)
		{
			for (int c = 0; c < 4; c++)
			{
				std::cout << v[r][c] << " ";
			}
			std::cout << std::endl;
		}
	}

};






// float alpha = M_PI / 4;
// float near = 0.0005;
// float far = 0.015;
#define alpha (M_PI / 4)
#define near (0.0005)
#define far (0.015)
#define NEAR (near * cosf(alpha))

class D3
{

	// create an empty shape
	sf::ConvexShape convex;

	int w_;
	int h_;

public:

	std::vector<D3Vec> arr;

	D3(int w, int h)
	{
		w_ = w;
		h_ = h;


#if 0
		// resize it to 5 points
		convex.setPointCount(5);

		// define the points
		convex.setPoint(0, sf::Vector2f(0, 0));
		convex.setPoint(1, sf::Vector2f(150, 10));
		convex.setPoint(2, sf::Vector2f(120, 90));
		convex.setPoint(3, sf::Vector2f(30, 100));
		convex.setPoint(4, sf::Vector2f(0, 50));
#endif
	}

	void draw(sf::RenderWindow& bkg)
	{
		if (arr.size() < 2)return;
		convex.setPointCount(arr.size());
		for (int i = 0; i < arr.size(); i++)
		{
			//float x = w_ / 2 * (1 + arr[i].v[0]);
			//float y = h_ / 2 * (1 + arr[i].v[1]);
			float z = arr[i].v[2];
			float xp = -arr[i].v[0] /z*NEAR;
			float yp = -arr[i].v[1] /z * NEAR * (w_/h_);
			float xb = w_ / 2 * (1 + xp);
			float yb = h_ / 2 * (1 + xp);



			//xb = w_ / 2 * (1 + arr[i].v[0]);
			xb = w_ / 2 * (1 - arr[i].v[0]/z*NEAR);
			yb = h_ / 2 * (1 - arr[i].v[1] / z * NEAR);



			sf::Vector2f sfvec(xb, yb);
			//sf::Vector2f sfvec(w_/2*(1 + arr[i].v[0]), h_ / 2 * (1 +arr[i].v[1]));
			//std::cout << i << " " << sfvec.x << " " << sfvec.y << std::endl;
			convex.setPoint(i, sfvec);
		}
		bkg.draw(convex);
	}
	void add(D3Vec v)
	{
		arr.push_back(v);
	}

	void addxy(float x, float y)
	{
		D3Vec d;
		d.v[0] = x;
		d.v[1] = y;
		d.v[2] = 0.0;
		d.v[3] = 1.0;
		arr.push_back(d);
	}

	void clear()
	{
		arr.clear();
	}
	
	void apply(D3Mat& mat)
	{
		for (int i = 0; i < arr.size(); i++)
		{
			std::cout << i << " before:" << std::endl;
			arr[i].print();
			arr[i] = mat.cross(arr[i]);
			std::cout << "after:" << std::endl;
			arr[i].print();
		}
	}


	

};




char getKey()
{

	static struct Keymap
	{

		sf::Keyboard::Key kcode;
		bool isPressed;
		const char * ckode_p;
	}keymap[] =
	{

		//{sf::Keyboard::A, false, "A"} // returns 'a'
		//{sf::Keyboard::B, false, "B"} // returns 'a'
#define KEYMAP(x) {sf::Keyboard::##x, false, #x}
		KEYMAP(A),
		KEYMAP(B),
		KEYMAP(C),
		KEYMAP(D),
		KEYMAP(E),
		KEYMAP(F),
		KEYMAP(G),
		KEYMAP(H),
		KEYMAP(I),
		KEYMAP(J),
		KEYMAP(K),
		KEYMAP(L),
		KEYMAP(M),
		KEYMAP(N),
		KEYMAP(O),
		KEYMAP(P),
		KEYMAP(Q),
		KEYMAP(R),
		KEYMAP(S),
		KEYMAP(T),
		KEYMAP(U),
		KEYMAP(V),
		KEYMAP(W),
		KEYMAP(X),
		KEYMAP(Y),
		KEYMAP(Z),


#undef KEYMAP
	};

	for (int i = 0; i < sizeof(keymap) / sizeof(Keymap); i++)
	{
		if (sf::Keyboard::isKeyPressed(keymap[i].kcode))
		{
			keymap[i].isPressed = true;
		}
		if (keymap[i].isPressed && !sf::Keyboard::isKeyPressed(keymap[i].kcode))
		{
			keymap[i].isPressed = false;
			return (*keymap[i].ckode_p - ('A' - 'a'));
		}
		
	}
	return '\0';
	//sf::Keyboard::isKeyPressed(sf::Keyboard::Space)

};

int main()
{


	D3Mat d = D3Mat::unity();
	std::cout << "------" << std::endl;
	//d.print();

	std::cout << "------" << std::endl;
	D3Vec v;
	//v.print();
	std::cout << "------" << std::endl;
	D3Vec v1;

	v1 = d.cross(v);

	//v1.print();
	std::cout << "------" << std::endl;
	D3Vec v2 = D3Vec::pos(0.1, 0.2, 0.0);
	//v2.print();
	std::cout << "------" << std::endl;

	D3Mat tr = D3Mat::trans(v2);
	std::cout << "translation" << std::endl;
	tr.print();



	Board board;
	//D3 d3;

	float jsx = 0.0; //TODO

	Screen screen;
	sf::RenderWindow window(screen.getVideoMode(), screen.getTitle(), screen.getStyle());

	window.setMouseCursorVisible(false);
	window.setVerticalSyncEnabled(true);


	sf::Clock clock; // starts the clock
	sf::Time timeSinceLastUpdate = sf::Time::Zero;


	D3 d3(screen.getWidth(), screen.getHeight());
	//d3.addxy(-0.1, -0.1);
	//d3.addxy(0.1, -0.1);
	//d3.addxy(0.1, 0.1);
	d3.add(D3Vec(-0.1, -0.1, 0.0, 1.0));
	d3.add(D3Vec(0.1, -0.1, 0.0, 1.0));
	d3.add(D3Vec(0.1, 0.1, 0.0, 1.0));


	//d3.addxy(-0.1, 0.1);

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
		text.draw(window);
		d3.draw(window);

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
			track.tick(dTime);
			sprintf(text.getBuf(), "% 4.2f", dTime / maxTimePerFrame);
			//printf("% 4.2f ", dTime / maxTimePerFrame);
			//tree.move(rfwd, rsw);
		}
		jsx = board.get();
		int printjsx = (int)jsx;


		static bool spacedone = false;


		switch (getKey())
		{

		case 'a':
		{
			std::cout << "a" << std::endl;
			D3Mat mat1 = D3Mat::trans(D3Vec(0.1, 0.0, -NEAR, 1.0));
			d3.apply(mat1);
			break;
		}
		case 'b':
		{
			std::cout << "b" << std::endl;
			break;
		}

		case 'x':
		{
			std::cout << "a" << std::endl;
			D3Mat mat1 = D3Mat::rotX(0.0001);
			d3.apply(mat1);
			break;
		}


		case 'y':
		{
			std::cout << "a" << std::endl;
			D3Mat mat1 = D3Mat::rotY(0.0001);
			d3.apply(mat1);
			break;
		}


		case 'z':
		{
			std::cout << "a" << std::endl;
			D3Mat mat1 = D3Mat::rotZ(M_PI / 32.0);
			d3.apply(mat1);
			break;
		}

		}; //switch



#if 0
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
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !spacedone) {
			spacedone = true;
			//D3Mat mat = D3Mat::scale(D3Vec(0.5, 3.0, 0.0, 0.0));
			//D3Mat mat = D3Mat::rotZ(-M_PI / 8);

			D3Mat mat1 = D3Mat::trans(D3Vec(0.1, 0.0, -NEAR, 1.0));
			D3Mat mat2 = D3Mat::D3Mat::rotX(0.0);

			for (int i = 0; i < d3.arr.size(); i++)
			{
				std::cout << i << " before:" << std::endl;
				d3.arr[i].print();
				//d3.arr[i] = tr.cross(d3.arr[i]);
				d3.arr[i] = mat1.cross(mat2.cross(d3.arr[i]));
				std::cout << "after:" << std::endl;
				d3.arr[i].print();

			}
		}




		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::T)) {
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
			{
				track.showTrack_ = false;
			}
			else {
				track.showTrack_ = true;
			}
		}



		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::P)) {
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
			{
				track.paintTrack_ = false;
			}
			else {
				track.paintTrack_ = true;
			}
		}
#endif






	}
	return 0;
}
