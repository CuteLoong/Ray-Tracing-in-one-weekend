# Ray Tracing in one Weekend

写在开头：

虽然作者说一个周末就能实现一个基于近似的、球形物体的ray tracer，虽然说不难，我最终还是花了4天的时间来做这个，不过第一次手动实现了一个渲染器，自我感觉还是不错的🤣。主要是了解的内容如下：

1. 了解了ppm的图片格式
2. 学习了RGB相关内容
3. 复习了c++中的const函数定义、虚函数与纯虚函数、运算符重载
4. 复习了光线的定义
5. 理解了viewport与真实图片的对应关系
6. 理解了光线的发出过程（ray是怎么打入场景中去的）
7. 学会了球形相关的内容（球的基本定义，如何相交，怎么在场景中显示球）
8. 学习了hittable物体的一系列属性（法线，内部外部，内涵碰撞检测）
9. 抗锯齿（主要是SSAA与MSAA的复习）
10. 材质的定义（lambertian(完美的漫反射)，metal，dielectrics）
11. 光线的反射、折射（并不是物理意义的，大多都是近似的）
12. 漫反射中采样问题（反射到发出点了）
13. metal物体反射时的fuzzy程度，以及reflect的函数
14. 折射的snell定则，以及snell定则无法解决的全反射(Total Internal Reflection)问题
15. 菲涅尔项，以及为了简便而采取的schilick近似
16. 视口变化相关的复习
17. 散焦模糊（深景）的初步学习
18. 以及很多很多的英语单词

下面就再说一说这些都是啥玩意吧！！



## PPM图片格式

大概长如下形式

```ppm
P3
3 2
255
# The part above is the header
# "P3" means this is a RGB color image in ASCII
# "3 2" is the width and height of the image in pixels
# "255" is the maximum value for each color
# The part below is image data: RGB triplets
255   0   0  # red
  0 255   0  # green
  0   0 255  # blue
255 255   0  # yellow
255 255 255  # white
  0   0   0  # black
```



## RGB相关内容

rgb通道在0-255是由于我们一般使用的通道是8位单通道，即
$$
2^8=256
$$
0-255正好256中颜色

映射到0-1是因为节省存储的空间，之后存储的时候在转化成舍弃精度的值即可

```cpp
ir = static_cast<int>(255.999 * r);
```

在使用8位单通道的RGB时，在做颜色的映射时，要注意保存的值不能大于255，否则会出现过多的噪点，而且结果将不正确，这种情况一般使用一个clamp限制

```cpp
inline double clamp(double x, double min, double max)
{
    if (x < min)
        return min;
    if (x > max)
        return max;
    return x;
}
```



## C++相关

对于一个const的对象，调用它的函数或它使用的函数需要被定义为const（具体我也还没翻书啊，初步实践是这样）

虚函数与纯虚函数：相当于java里面的抽象和接口吧，定义的方法是纯虚函数后面加一个=0

运算符重载：在类内部重载时，会自动传入一个this指针，代表当前元素。

友元函数：如果外部函数需要使用内部private的属性，则可以定义成友元

类成员函数定义：

> 如果在类的外部定义内部声明的函数，一定要加class::funcName
>
> 定义static函数，仅需要在声明的时候声明static即可

随机数的定义：

定义一个连续均匀分布类模板**uniform_real_distribution**

or定义一个离散均匀分布类模板**uniform_int_distribution**

定义一个伪随机数生成器**mt19937**



## 光线的定义

ray的定义：
$$
\vec{O}+t\vec{d}
$$

光线打出时，会计算当前的点，以及出射位置，分为两种情况:

1. 就是在观测点打出

   > 光线打出时，用视口上的一个点减去观测点坐标，得到一个方向向量即可

2. 反射或折射打出

光线打出时要注意光线的能量变化

关于光线反射的次数：

> 1. 可以自己定义最大的反射次数，衰减率就是当前物体的albedo
> 2. 通过俄罗斯轮盘赌，衰减率是albedo/p（正好期望等于albedo）



## ViewPort相关

viewport本质上是不存在的，可以被看做是一个虚拟的（真实）世界，存在的意义，在我看来有以下几点：

> 1. 用于光线的发射
> 2. 计算着色时需要
> 3. 所有物体基本单位与视口保持一致，相当于一个世界的基准

viewport与最终的图像关系：

> 1. 图片的像素与视口无关，只与图片本身的定义有关
> 2. 图片的像素多少意味着将会把视口分为多少份（分的份数越多当然就越细腻啦）
> 3. 视口的大小可以说就是视角的大小（视口长宽固定时，提高距离就是变相把视口缩小），和focal-length、fov有关（当然focal-length只是相对的有关，即如果视口大小不变，而focal-length变大，视野自然就小了），将会影响最终图片的视野
> 4. 视口的宽高比要与图像的宽高比一致（这是当然的罗，不然怎么对应划分）

对viewport的一些理解：

> 1. 视口的位置与观测点有关
> 2. 视口的高一般设为2个单位长度*fov角度的tan值，fov小观测空间自然就小
> 3. focal-length一般都会设为1个单位
> 4. 视口的位置基本上可以代表从哪个方位渲染画面
> 5. 观测点，一定对应视口的中心



## Sphere相关

关于相交：

> 光线与球的相交比较简明，所以大家都喜欢用球来做光线追踪的物体
>
> 由于计算需要，我们将球的公式转化为以向量为term的formulas
> $$
> (\vec{P}-\vec{C})·(\vec{P}-\vec{C})=(x-C_x)^2+(y-C_y)^2+(z-C_z)^2=>\\(\vec{P}-\vec{C})·(\vec{P}-\vec{C})=r^2=>\\((\vec{O}+t\vec{d})-\vec{C})·((\vec{O}+t\vec{d})-\vec{C}) = r^2=>\\t^2\vec{d}·\vec{d}+2t\vec{d}·(\vec{O}-\vec{C})+(\vec{O}-\vec{C})·(\vec{O}-\vec{C})-r^2=0
> $$
> 根据上述公式最后接出来**t**即可
>
> 判断有无hit直接使用判别式判定就行了

在场景中显示球：

> 需要与所有的球都求交，找出最近的hit点，读出该点的材质，并继续做光线追踪，最终求出所有交点的加权平均即可
>
> 如果只是需要显示单独的一个球，使用最近的hit点给该像素shade即可



## Hittable物体

hittable当然就是可以与光发生一定作用的意思罗

当然，就此原因，我们就会把碰撞的一系列函数定义在这种物体的类里面（当然如果是一个hittable_list需要的碰撞函数，就是找到最近的那个点了）

既然是光的碰撞，那自然避不开法线，所以需要计算出来法线嘞

> 在这里，需要判断法线的方向，如果在内部仍然使用向外的法线自然是错的



与物体相交时，需要注意一个**自相交**问题，如果t_min选择为0.0，计算得出的t虽然也是0，但是实际在计算机中可能是0.00000001，那光线就会在原地打转了

即需要处理阴影失真的问题

>导致这个问题的原因有两种：
>
>1. 精度不够，导致自相交等问题
>2. 在shadowmap里面，深度map的走样问题（深度的采样频率不够）
>
>解决方案：
>
>​	精度问题：忽略掉**t**处于0附近的值
>
>​	深度map走样问题：一般加入一个bias即可



## antialiasing(抗锯齿，反走样)

aliasing走样

### SSAA

相当于以一张更大的分辨率来渲染场景，提高采样率，需要采样所有采样点的颜色（这个导致需要先把每个片段的颜色先shader才能算出来，计算量巨大），最后平均得到像素的颜色

### MSAA

也是相当于在一张更大的分辨率来渲染场景，提高了采样率，不过只需要覆盖测试以及深度测试，及当前像素点的颜色，以及提高分辨率后的覆盖率即可计算出当前像素的颜色



## 材质定义

lambertian（完美的漫反射）：图不好放

> 从hit点延申到法线单位圆，使用lambertian反射方法，取一个长度为单位长度的向量打到圆上，从而确定最终的出射方向，这种方法的分布为cos(φ)
>
> **注意：**
>
> 取得的单位向量可能会打回到原来的点，所以需要对这种情况进行处理
>
> ```cpp
> bool vec3::near_zero() const
> {
>     const auto s = 1e-8;
>     return (fabs(e[0]) < s) && (fabs(e[1]) < s) && (fabs(e[2]) < s);
> }
> ```



metal材质（反射）：

> 关于光线的反射
>
> ```cpp
> vec3 reflect(const vec3 &v, const vec3 &n)
> {
>     return v - 2 * dot(v, n) * n;
> }
> ```
>
> 再给金属球做反射时，为了让球看起来并不是完美的光滑，需要让他fuzzy一些
>
> ```cpp
> scattered = ray(rec.p, reflected + fuzz*random_in_unit_sphere());
> ```



dielectrics材质（折射）：以glass为代表

>对于一次光线的交会，只进行折射（refraction）或只进行反射（reflection）
>
>典型的折射率：air:1.0	glass:1.3-1.7	diamond:2.4
>
>可以将折射的光线看成平行于n'的向量于垂直于n'向量的和(下列公式都是向量，我懒得打了)
>$$
>R'=R'_{\bot} + R'_{\parallel}\\
>because:\sin{θ'}=\frac{\eta}{\eta'}\sin{θ}\\
>so:R'_{\bot}=\frac{\eta}{\eta'}(R+\cos{\theta}\vec{n})=>R'_{\bot}=\frac{\eta}{\eta'}(R+(-\vec{R}·\vec{n})\vec{n})\\
>R'_{\parallel}=-\sqrt{1-R'^2_{\bot}}·\vec{n}
>$$
>
>```cpp
>vec3 refract(const vec3 &uv, const vec3 &n, double etai_over_etat)
>{
>    auto cos_theta = fmin(dot(uv, n), 1.0); // i guess this is because dot result maybe over 1.0, such as 1.0000001
>    vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
>    vec3 r_out_parallel = -sqrt(fabs(1 - r_out_perp.length_squared())) * n;
>    return r_out_perp + r_out_parallel;
>}
>```
>
>snell解决不了全内反射（total internal reflection）
>
>由于sinθ要小于1，所以snell low只能解决一部分折射，当从高折射率截至转入低折射率介质时，就完蛋了
>
>即当右边大于1时，需要控制进行反射
>$$
>\sin{θ'}=\frac{\eta}{\eta'}\sin{θ}
>$$



## Fresnel equation相关

菲涅耳方程（Fresnel equation）描述了光线经过两个介质的界面时，反射和透射的光强比重。

分别对应入射光的 s 偏振（senkrecht polarized）和 p 偏振（parallel polarized）所造成的反射比。图形学中通常考虑光是无偏振的（unpolarized），也就是两种偏振是等量的，所以可以取其平均值：
$$
R_s=(\frac{\eta_1\cos{\theta_i}-\eta_2\cos{\theta_t}}{\eta_1\cos{\theta_i}+\eta_2\cos{\theta_t}})^2\\R_p=(\frac{\eta_1\cos{\theta_t}-\eta_2\cos{\theta_i}}{\eta_1\cos{\theta_t}+\eta_2\cos{\theta_i}})^2\\R=\frac{R_s+R_p}{2}
$$
但是，这种方法太复杂了，所以这里使用的是另一种近似方法

Schilick近似

计算出的结果为反射比，即有多少光线是被反射的占比，剩下T=1-R
$$
R(\theta_i) ≈ R(0) +  (1 - R(0))(1-\cos{\theta_i})^5
$$

$$
R(0)=\frac{\eta_i - \eta_t}{\eta_i+\eta_t}
$$



## 散焦模糊（defocus blur）

散焦模糊，用另一个摄影的术语来说就是——深景

仔细体会看东西的时候，只有我们关注的点会很清晰，而我们不关注的点会变得模糊

这其实和人眼构造有关系——人眼相当于一个棱镜

**距离注视的平面越远，光圈的大小越大，自然越模糊**

所以在渲染的时候我们也应该注意这种模糊，即加入了一个focus-distance（成像距离，注意与focal-length区分）与apertrue（光圈大小），这时，视口又有了一个新的含义，就是聚焦的位置平面，离这个平面越远当然这个点就越模糊了.

注意，为了保证视口的相对大小不变，需要做如下变化

```cpp
horizontal = focus_dist * viewport_width * u;
vertical = focus_dist * viewport_height * v;
```

这里暂时还只是了解，并没有进行深入学习



就写到这吧，happy-end，结果图如下：

![image](E:\git warehouse\Ray-Tracing-in-one-weekend\image\image.jpg)