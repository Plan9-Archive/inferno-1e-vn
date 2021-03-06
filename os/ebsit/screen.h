typedef struct Cursor Cursor;
struct	Cursor
{
	Point	offset;
	uchar	clr[2*16];
	uchar	set[2*16];
};

typedef struct Vctlr Vctlr;
struct Vctlr {
	char*	name;
	Vctlr*	(*init)(Vctlr*, int, int, int);
	void	(*page)(int);
	void	(*setcolor)(ulong, ulong, ulong, ulong);

	void	(*enable)(void);
	void	(*disable)(void);
	int	(*move)(int, int);
	void	(*load)(Cursor*);
	int	(*isloaded)(Cursor*);
	int	(*cursorintersectsoff)(Rectangle*);

	int	x;
	int	y;
	int	d;

	uchar*	aperture;
	int	apsize;
	int	apshift;
	int	linear;

	Vctlr*	link;

	int	showcursor;
	int	loaded;
	Cursor	cursor;
	Lock	l;
};


