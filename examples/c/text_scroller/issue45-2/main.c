void initvideomode();
void loadobj();
void initpxyattic();
void initangles();
void projectallvertices();
int getkey();

int main() {
    //initvideomode();

    while (1) {

        if (getkey() != 0) {

            loadobj();
			initpxyattic();
			initangles();
			projectallvertices();

			while (1) {

		    }

        }

    }

}