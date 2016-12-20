class InterpolationObject{
public:
	InterpolationObject();
	~InterpolationObject();

	void initInterpolationObject(int _n, double _x0, double dx, double* data);
	void bcastInterpolationObject(int rank);
	void interpolatefile();
	int n;          //!< �������ݸ���
	double x0;      //!< ��ʼ��
	double invDx;   //!< ����
	double* values;
	double** spline;
};