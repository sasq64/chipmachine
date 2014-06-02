#ifndef LIST_H
#define LIST_H

struct Rectangle {
	Rectangle() {}
	Rectangle(float w, float h) : x(0), y(0), w(w), h(h), rot(0.0) {}
	Rectangle(float x, float y, float w, float h, float rot = 0.0) : x(x), y(y), w(w), h(h), rot(rot) {}
	union {
		float p[5];
		struct {
			float x;
			float y;
			float w;
			float h;
			float rot;
		};
	};
	float& operator[](const int &index) { return p[index]; }

	Rectangle operator/(const Rectangle &r) const {
		return Rectangle(x, y, w / r.w, h / r.h);
	}

};

class VerticalLayout {
public:
	VerticalLayout(const Rectangle &screenArea, int visibleItems) : screenArea(screenArea), visibleItems(visibleItems) {
		itemSize = screenArea / Rectangle(1, visibleItems);
	}

	Rectangle layout(float position) {
		Rectangle res = itemSize;
		res.y = (screenArea.h - itemSize.y) * position;
		return res;
	}
private:
	Rectangle screenArea;
	int visibleItems;
	Rectangle itemSize;
};

template <typename LAYOUT> class Base_List {
public:

	struct Renderer {
		virtual void render_item(Rectangle &rec, int y, uint64_t index, bool hilight) = 0;
	};


	Base_List(std::function<void(Rectangle &rec, int y, uint64_t index, bool hilight)> renderFunc, const Rectangle &area, int visibleItems) : renderFunc(renderFunc), visibleItems(visibleItems), layout(area, visibleItems) {
	}

	Base_List(Renderer *renderer, const Rectangle &area, int visibleItems) : renderer(renderer), visibleItems(visibleItems), layout(area, visibleItems) {
	}

	void render() {
		if(renderer != 0) {
			for(int i=0; i<visibleItems; i++) {
				auto rec = layout.layout(i / (float)visibleItems);
				renderer->render_item(rec, i, i + position, false);
			}
		} else {
			for(int i=0; i<visibleItems; i++) {
				auto rec = layout.layout(i / (float)visibleItems);
				renderFunc(rec, i, i + position, false);
			}
		}
	}

	void select(uint64_t index) {
		selected_item = index;
		if(selected_item < position)
			position = selected_item;
		if(selected_item >= position+visibleItems)
			position = selected_item-visibleItems+1;
	}

	int selected() { return selected_item; }


private:
	Renderer *renderer;
	std::function<void(Rectangle &rec, int y, uint64_t index, bool hilight)> renderFunc;
	int visibleItems;
	LAYOUT layout;
	uint64_t position = 0;
	uint64_t selected_item = 0;
};


typedef Base_List<VerticalLayout> VerticalList;


#endif // LIST_H
