#pragma once

namespace clipper4CLR {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Drawing::Drawing2D;
	using namespace clipper4;
	using namespace System::Runtime::InteropServices;
	
	/// <summary>
	/// Summary for Form1
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
	public:
		Form1(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Panel^  panel1;
	protected: 
	private: System::Windows::Forms::Button^  bClose;
	private: System::Windows::Forms::Button^  bRefresh;

	private: System::Windows::Forms::Panel^  panel2;
	private: System::Windows::Forms::PictureBox^  pictureBox1;
	private: System::Windows::Forms::GroupBox^  groupBox1;
	private: System::Windows::Forms::RadioButton^  rbNonZero;
	private: System::Windows::Forms::RadioButton^  rbEvenOdd;
	private: System::Windows::Forms::OpenFileDialog^  openFileDialog1;
	private: System::Windows::Forms::Button^  button1;
	private: System::Windows::Forms::Button^  button2;
	private: System::Windows::Forms::GroupBox^  groupBox2;
	private: System::Windows::Forms::RadioButton^  rbXor;
	private: System::Windows::Forms::RadioButton^  rbDifference;
	private: System::Windows::Forms::RadioButton^  rbUnion;
	private: System::Windows::Forms::RadioButton^  rbIntersection;



	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->panel1 = (gcnew System::Windows::Forms::Panel());
			this->groupBox2 = (gcnew System::Windows::Forms::GroupBox());
			this->rbXor = (gcnew System::Windows::Forms::RadioButton());
			this->rbDifference = (gcnew System::Windows::Forms::RadioButton());
			this->rbUnion = (gcnew System::Windows::Forms::RadioButton());
			this->rbIntersection = (gcnew System::Windows::Forms::RadioButton());
			this->button2 = (gcnew System::Windows::Forms::Button());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->groupBox1 = (gcnew System::Windows::Forms::GroupBox());
			this->rbNonZero = (gcnew System::Windows::Forms::RadioButton());
			this->rbEvenOdd = (gcnew System::Windows::Forms::RadioButton());
			this->bClose = (gcnew System::Windows::Forms::Button());
			this->bRefresh = (gcnew System::Windows::Forms::Button());
			this->panel2 = (gcnew System::Windows::Forms::Panel());
			this->pictureBox1 = (gcnew System::Windows::Forms::PictureBox());
			this->openFileDialog1 = (gcnew System::Windows::Forms::OpenFileDialog());
			this->panel1->SuspendLayout();
			this->groupBox2->SuspendLayout();
			this->groupBox1->SuspendLayout();
			this->panel2->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->pictureBox1))->BeginInit();
			this->SuspendLayout();
			// 
			// panel1
			// 
			this->panel1->BorderStyle = System::Windows::Forms::BorderStyle::Fixed3D;
			this->panel1->Controls->Add(this->groupBox2);
			this->panel1->Controls->Add(this->button2);
			this->panel1->Controls->Add(this->button1);
			this->panel1->Controls->Add(this->groupBox1);
			this->panel1->Controls->Add(this->bClose);
			this->panel1->Controls->Add(this->bRefresh);
			this->panel1->Dock = System::Windows::Forms::DockStyle::Left;
			this->panel1->Location = System::Drawing::Point(0, 0);
			this->panel1->Name = L"panel1";
			this->panel1->Size = System::Drawing::Size(144, 447);
			this->panel1->TabIndex = 0;
			// 
			// groupBox2
			// 
			this->groupBox2->Controls->Add(this->rbXor);
			this->groupBox2->Controls->Add(this->rbDifference);
			this->groupBox2->Controls->Add(this->rbUnion);
			this->groupBox2->Controls->Add(this->rbIntersection);
			this->groupBox2->Location = System::Drawing::Point(17, 143);
			this->groupBox2->Name = L"groupBox2";
			this->groupBox2->Size = System::Drawing::Size(99, 119);
			this->groupBox2->TabIndex = 2;
			this->groupBox2->TabStop = false;
			this->groupBox2->Text = L"&Boolean";
			// 
			// rbXor
			// 
			this->rbXor->AutoSize = true;
			this->rbXor->Location = System::Drawing::Point(12, 87);
			this->rbXor->Name = L"rbXor";
			this->rbXor->Size = System::Drawing::Size(48, 17);
			this->rbXor->TabIndex = 3;
			this->rbXor->Text = L"XOR";
			this->rbXor->UseVisualStyleBackColor = true;
			this->rbXor->Click += gcnew System::EventHandler(this, &Form1::rbNonZero_Click);
			// 
			// rbDifference
			// 
			this->rbDifference->AutoSize = true;
			this->rbDifference->Location = System::Drawing::Point(12, 65);
			this->rbDifference->Name = L"rbDifference";
			this->rbDifference->Size = System::Drawing::Size(74, 17);
			this->rbDifference->TabIndex = 2;
			this->rbDifference->Text = L"Difference";
			this->rbDifference->UseVisualStyleBackColor = true;
			this->rbDifference->Click += gcnew System::EventHandler(this, &Form1::rbNonZero_Click);
			// 
			// rbUnion
			// 
			this->rbUnion->AutoSize = true;
			this->rbUnion->Location = System::Drawing::Point(12, 43);
			this->rbUnion->Name = L"rbUnion";
			this->rbUnion->Size = System::Drawing::Size(53, 17);
			this->rbUnion->TabIndex = 1;
			this->rbUnion->Text = L"Union";
			this->rbUnion->UseVisualStyleBackColor = true;
			this->rbUnion->Click += gcnew System::EventHandler(this, &Form1::rbNonZero_Click);
			// 
			// rbIntersection
			// 
			this->rbIntersection->AutoSize = true;
			this->rbIntersection->Checked = true;
			this->rbIntersection->Location = System::Drawing::Point(12, 21);
			this->rbIntersection->Name = L"rbIntersection";
			this->rbIntersection->Size = System::Drawing::Size(80, 17);
			this->rbIntersection->TabIndex = 0;
			this->rbIntersection->TabStop = true;
			this->rbIntersection->Text = L"Intersection";
			this->rbIntersection->UseVisualStyleBackColor = true;
			this->rbIntersection->Click += gcnew System::EventHandler(this, &Form1::rbNonZero_Click);
			// 
			// button2
			// 
			this->button2->Location = System::Drawing::Point(17, 310);
			this->button2->Name = L"button2";
			this->button2->Size = System::Drawing::Size(99, 23);
			this->button2->TabIndex = 4;
			this->button2->Text = L"&Open &Clip File ...";
			this->button2->UseVisualStyleBackColor = true;
			this->button2->Click += gcnew System::EventHandler(this, &Form1::button1_Click);
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(17, 279);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(99, 23);
			this->button1->TabIndex = 3;
			this->button1->Text = L"&Open &Subj File ...";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &Form1::button1_Click);
			// 
			// groupBox1
			// 
			this->groupBox1->Controls->Add(this->rbNonZero);
			this->groupBox1->Controls->Add(this->rbEvenOdd);
			this->groupBox1->Location = System::Drawing::Point(17, 59);
			this->groupBox1->Name = L"groupBox1";
			this->groupBox1->Size = System::Drawing::Size(99, 74);
			this->groupBox1->TabIndex = 1;
			this->groupBox1->TabStop = false;
			this->groupBox1->Text = L"&FillMode";
			// 
			// rbNonZero
			// 
			this->rbNonZero->AutoSize = true;
			this->rbNonZero->Location = System::Drawing::Point(12, 42);
			this->rbNonZero->Name = L"rbNonZero";
			this->rbNonZero->Size = System::Drawing::Size(67, 17);
			this->rbNonZero->TabIndex = 1;
			this->rbNonZero->TabStop = true;
			this->rbNonZero->Text = L"NonZero";
			this->rbNonZero->UseVisualStyleBackColor = true;
			this->rbNonZero->Click += gcnew System::EventHandler(this, &Form1::rbNonZero_Click);
			// 
			// rbEvenOdd
			// 
			this->rbEvenOdd->AutoSize = true;
			this->rbEvenOdd->Checked = true;
			this->rbEvenOdd->Location = System::Drawing::Point(11, 21);
			this->rbEvenOdd->Name = L"rbEvenOdd";
			this->rbEvenOdd->Size = System::Drawing::Size(70, 17);
			this->rbEvenOdd->TabIndex = 0;
			this->rbEvenOdd->TabStop = true;
			this->rbEvenOdd->Text = L"EvenOdd";
			this->rbEvenOdd->UseVisualStyleBackColor = true;
			this->rbEvenOdd->Click += gcnew System::EventHandler(this, &Form1::rbNonZero_Click);
			// 
			// bClose
			// 
			this->bClose->DialogResult = System::Windows::Forms::DialogResult::Cancel;
			this->bClose->Location = System::Drawing::Point(17, 366);
			this->bClose->Name = L"bClose";
			this->bClose->Size = System::Drawing::Size(99, 23);
			this->bClose->TabIndex = 5;
			this->bClose->Text = L"E&xit";
			this->bClose->UseVisualStyleBackColor = true;
			this->bClose->Click += gcnew System::EventHandler(this, &Form1::bClose_Click);
			// 
			// bRefresh
			// 
			this->bRefresh->Location = System::Drawing::Point(17, 25);
			this->bRefresh->Name = L"bRefresh";
			this->bRefresh->Size = System::Drawing::Size(99, 23);
			this->bRefresh->TabIndex = 0;
			this->bRefresh->Text = L"&Random";
			this->bRefresh->UseVisualStyleBackColor = true;
			this->bRefresh->Click += gcnew System::EventHandler(this, &Form1::bRefresh_Click);
			// 
			// panel2
			// 
			this->panel2->BorderStyle = System::Windows::Forms::BorderStyle::Fixed3D;
			this->panel2->Controls->Add(this->pictureBox1);
			this->panel2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panel2->Location = System::Drawing::Point(144, 0);
			this->panel2->Name = L"panel2";
			this->panel2->Size = System::Drawing::Size(601, 447);
			this->panel2->TabIndex = 1;
			// 
			// pictureBox1
			// 
			this->pictureBox1->BackColor = System::Drawing::Color::White;
			this->pictureBox1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->pictureBox1->Location = System::Drawing::Point(0, 0);
			this->pictureBox1->Name = L"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(597, 443);
			this->pictureBox1->TabIndex = 2;
			this->pictureBox1->TabStop = false;
			this->pictureBox1->Paint += gcnew System::Windows::Forms::PaintEventHandler(this, &Form1::pictureBox1_Paint);
			// 
			// openFileDialog1
			// 
			this->openFileDialog1->DefaultExt = L"txt";
			this->openFileDialog1->FileName = L"openFileDialog1";
			this->openFileDialog1->Title = L"Open Clipper File";
			// 
			// Form1
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->CancelButton = this->bClose;
			this->ClientSize = System::Drawing::Size(745, 447);
			this->Controls->Add(this->panel2);
			this->Controls->Add(this->panel1);
			this->Name = L"Form1";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"Clipper C++/CLR Demo";
			this->FormClosed += gcnew System::Windows::Forms::FormClosedEventHandler(this, &Form1::Form1_FormClosed);
			this->Load += gcnew System::EventHandler(this, &Form1::Form1_Load);
			this->panel1->ResumeLayout(false);
			this->groupBox2->ResumeLayout(false);
			this->groupBox2->PerformLayout();
			this->groupBox1->ResumeLayout(false);
			this->groupBox1->PerformLayout();
			this->panel2->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->pictureBox1))->EndInit();
			this->ResumeLayout(false);

		}
#pragma endregion

		static Polygons *subj, *clip, *solution;
		static PolyFillType pft = pftEvenOdd;
		static ClipType ct = ctIntersection;
		static int scaling = 1;
		static bool succeeded = false;

		private: System::Void MakeRandomPoly(Polygons &pgs, int width, int height, int edgeCount)
		{
			pgs.resize(1);
			pgs[0].resize(edgeCount);
			for (int i = 0; i < edgeCount; i++)
			{
				pgs[0][i].X = rand()%(width-20) +10;
				pgs[0][i].Y = rand()%(height-20) +10;
			}
		}
		//------------------------------------------------------------------------------

		private: System::Void MakePolygon(Polygon& pg, int* pts, int cnt)
		{
			pg.clear();
			  pg.resize(cnt/2);
			  int x,y;
			  for (int i = 0; i < cnt/2; ++i) {
				x = *pts++; y = *pts++;
				pg[i].X = x;
				pg[i].Y = y;
			  }
			}
		//---------------------------------------------------------------------------

		private: System::Void MakeNewPolygons(bool solutionOnly)
		{
				 //int pts[] = {380, 10,  320, 370,  570, 70}; 
				 //subj->resize(1);
				 //MakePolygon((*subj)[0], pts, sizeof(pts)/sizeof(pts[0]));

				if (!solutionOnly)
				{
					MakeRandomPoly(*subj, pictureBox1->Size.Width, this->pictureBox1->Size.Height, 50);
					MakeRandomPoly(*clip, pictureBox1->Size.Width, this->pictureBox1->Size.Height, 50);
				}

				Clipper4 *cp4 = new Clipper4;
				cp4->AddPolygons(*subj, ptSubject);
				cp4->AddPolygons(*clip, ptClip);
				succeeded = cp4->Execute(ct, *solution, pft, pft);
				delete cp4;
				if (!succeeded) solution->clear();
		}
		//---------------------------------------------------------------------------

		private: System::Void DrawPolygons(Polygons &ppg, Graphics ^ g, Pen ^ p, Brush ^ b)
		{
            GraphicsPath ^ path = gcnew GraphicsPath;
			if (pft == pftNonZero)
				path->FillMode = FillMode::Winding; else
				path->FillMode = FillMode::Alternate; 
			for (unsigned i = 0; i < ppg.size(); ++i)
				if (ppg[i].size() > 2)
					{
					array<Point> ^ pts = gcnew array<Point>(ppg[i].size());
					for (unsigned j = 0; j < ppg[i].size(); ++j)
					{
						pts[j].X = ppg[i][j].X;
						pts[j].Y = ppg[i][j].Y;
					}
					path->AddPolygon(pts);
					}
			g->FillPath( b, path);
			g->DrawPath( p, path);
		}
		//---------------------------------------------------------------------------
			 
		inline int Round(double val)
		{
		  if ((val < 0)) return (int)(val - 0.5); else return (int)(val + 0.5);
		}
		//------------------------------------------------------------------------------

		private: bool LoadFromFile(char* filename, clipper4::Polygons &pp){
		  char buffer[10];
		  FILE *f; 
		  fopen_s(&f, filename, "r");
		  if (f == 0) return false;
		  int polyCnt, vertCnt, i, j;
		  double X, Y;
		  pp.clear();
		  if (fscanf_s(f, "%d", &polyCnt) != 1 || polyCnt == 0) return false;
		  pp.resize(polyCnt);
		  for (i = 0; i < polyCnt; i++) {
			if (fscanf_s(f, "%d", &vertCnt) != 1) return false;
			pp[i].resize(vertCnt);
			for (j = 0; j < vertCnt; j++){
			  if (fscanf_s(f, "%lf, %lf", &X, &Y) != 2) return false;
			  pp[i][j].X = Round(X *scaling); pp[i][j].Y = Round(Y *scaling);
			  fgets(buffer, 10, f); //gobble any trailing commas
			}
		  }
		  fclose(f);
		  return true;
		}
		//---------------------------------------------------------------------------

	private: System::Void bClose_Click(System::Object^  sender, System::EventArgs^  e) {
				 Close();
			 }

	private: System::Void pictureBox1_Paint(System::Object^  sender, System::Windows::Forms::PaintEventArgs^  e) {
				 Graphics ^ g = e->Graphics;
				 g->SmoothingMode = System::Drawing::Drawing2D::SmoothingMode::AntiAlias;
				 
				 Pen^ p = gcnew Pen( Color::Black, 1.5f );
				 SolidBrush^ b = gcnew SolidBrush( Color::FromArgb( 0x200000FF) );
				 DrawPolygons(*subj, g , p, b);
				 b->Color = Color::FromArgb(0x2FF00000);
				 DrawPolygons(*clip, g , p, b);

				if (succeeded)
				{
					b->Color = Color::FromArgb(0x800FF000);
					DrawPolygons(*solution, g , p, b);
				} else
				{
					System::Drawing::Font ^ fnt = gcnew System::Drawing::Font("Arial",8);
					g->DrawString("Something went wrong!", fnt, System::Drawing::Brushes::Navy, Point(10,10));
				}
			 }

	private: System::Void Form1_Load(System::Object^  sender, System::EventArgs^  e) {
			srand((unsigned)time(0));
			subj = new clipper4::Polygons;
			clip = new clipper4::Polygons;
			solution = new clipper4::Polygons;
			MakeNewPolygons(false);
		 }

	private: System::Void Form1_FormClosed(System::Object^  sender, System::Windows::Forms::FormClosedEventArgs^  e) {
			delete subj;
			delete clip;
			delete solution;
		 }
private: System::Void bRefresh_Click(System::Object^  sender, System::EventArgs^  e) {
			MakeNewPolygons(false);
			pictureBox1->Invalidate();
		 }
private: System::Void rbNonZero_Click(System::Object^  sender, System::EventArgs^  e) {
			if (rbNonZero->Checked) pft = pftNonZero; else pft = pftEvenOdd;
			if (rbIntersection->Checked) ct = ctIntersection;
			else if (rbUnion->Checked) ct = ctUnion;
			else if (rbDifference->Checked) ct = ctDifference;
			else ct = ctXor;
			MakeNewPolygons(true);
			pictureBox1->Invalidate();
		 }

private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {
			 if (openFileDialog1->ShowDialog() != System::Windows::Forms::DialogResult::OK) return;
			 char *str = (char*)(void*)Marshal::StringToHGlobalAnsi(openFileDialog1->FileName);
			 if (sender == button1)
				LoadFromFile(str, *subj); else
				LoadFromFile(str, *clip);
			MakeNewPolygons(true);
			pictureBox1->Invalidate();
		 }
};

}

