#include "imageio.h"

ImageIO::ImageIO()
{

}

void ImageIO::save_image_bmp(cv::Mat img, QString filename)
{
    //    QPixmap::fromImage(QImage(img.data, img.cols, img.rows, img.step, img.channels() == 3 ? QImage::Format_RGB888 : QImage::Format_Indexed8)).save(filename + "_qt", "BMP", 100);

    if (img.channels() == 3) cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
    /*
    // using qt file io
    QFile f(filename);
    f.open(QIODevice::WriteOnly);
    if (f.isOpen()) {
//        qDebug() << filename;
        QDataStream out(&f);
        out.setByteOrder(QDataStream::LittleEndian);
        quint32_le offset((img.channels() == 1 ? 4 * (1 << img.channels() * 8) : 0) + 54);
        // bmp file header
        out << (quint16_le)'MB' << (quint32_le)(img.total() * img.channels() + offset) << (quint16_le)0 << (quint16_le)0 << offset;
        //  windows signature,  total file size,                                       reserved1,       reserved2,       offset to data
        // DIB header
        out << (quint32_le)40 << (quint32_le)img.cols << (quint32_le)img.rows << (quint16_le)1 << (quint16_le)(img.channels() * 8) << (quint32_le)0 << (quint32_le)(img.total() * img.channels()) << (quint32_le)0 << (quint32_le)0 << (quint32_le)0 << (quint32_le)0;
        //  dib header size,  width,                  height,                 planes,          bit per pixel,                      compression,     image size,                                   x pixels in meter, y,             color important, color used
        // color table
        if (img.channels() == 1) {
            for (int i = 0; i < 1 << img.channels() * 8; i++) out << (quint8)i << (quint8)i << (quint8)i << (quint8)0;
        //                                                        rgb blue,    rgb green,   rgb red,     reserved
        }
        for(int i = img.rows - 1; i >= 0; i--) f.write((char*)img.data + i * img.step, img.step);
        f.close();
    }
*/
    // using std file io
    FILE *f = fopen(filename.toLocal8Bit().constData(), "wb");
    if (f) {
        static ushort signature = 'MB';
        fwrite(&signature, 2, 1, f);
        uint offset = (img.channels() == 1 ? 4 * (1 << img.channels() * 8) : 0) + 54;
        uint file_size = img.total() * img.channels() + offset;
        fwrite(&file_size, 4, 1, f);
        static ushort reserved1 = 0, reserved2 = 0;
        fwrite(&reserved1, 2, 1, f);
        fwrite(&reserved2, 2, 1, f);
        fwrite(&offset, 4, 1, f);
        static uint dib_size = 40;
        fwrite(&dib_size, 4, 1, f);
        uint w = img.cols, h = img.rows;
        fwrite(&w, 4, 1, f);
        fwrite(&h, 4, 1, f);
        static ushort planes = 1;
        fwrite(&planes, 2, 1, f);
        ushort bit_per_pixel = 8 * img.channels();
        fwrite(&bit_per_pixel, 2, 1, f);
        static uint compression = 0;
        fwrite(&compression, 4, 1, f);
        uint img_size = img.total() * img.channels();
        fwrite(&img_size, 4, 1, f);
        static uint pixels_in_meter_x = 0, pixels_in_meter_y = 0;
        fwrite(&pixels_in_meter_x, 4, 1, f);
        fwrite(&pixels_in_meter_y, 4, 1, f);
        static uint colors_important = 0, colors_used = 0;
        fwrite(&colors_important, 4, 1, f);
        fwrite(&colors_used, 4, 1, f);

        // color table
        static uchar empty = 0;
        if (img.channels() == 1) {
            for (int i = 0; i < 1 << img.channels() * 8; i++) {
                fwrite(&i, 1, 1, f);     // r
                fwrite(&i, 1, 1, f);     // g
                fwrite(&i, 1, 1, f);     // b
                fwrite(&empty, 1, 1, f); // null
            }
        }
        static uint padding = 0;
        int padding_size = (4 - img.step % 4) % 4;
        for(int i = img.rows - 1; i >= 0; i--) fwrite(img.data + i * img.step, 1, img.step, f), fwrite(&padding, 1, padding_size, f);
        fclose(f);
    }
}

void ImageIO::save_image_tif(cv::Mat img, QString filename)
{
    uint32_t w = img.cols, h = img.rows, channel = img.channels();
    uint32_t depth = 8 * (img.depth() / 2 + 1);
    FILE *f = fopen(filename.toLocal8Bit().constData(), "wb");
    static uint16_t type_short = 3, type_long = 4, type_fraction = 5;
    static uint32_t count_1 = 1, count_3 = 3;
    static uint32_t TWO = 2;
    static uint32_t next_ifd_offset = 0;
    uint64_t block_size = w * channel, sum = h * block_size * depth / 8;
    uint i;

    // Offset=w*h*d + 8(eg:Img[1000][1000][3] --> 3000008)
    // RGB  Color:W H BitsPerSample Compression Photometric StripOffset Orientation SamplePerPixle RowsPerStrip StripByteCounts PlanarConfiguration
    // Gray Color:W H BitsPerSample Compression Photometric StripOffset Orientation SamplePerPixle RowsPerStrip StripByteCounts XResolution YResoulution PlanarConfiguration ResolutionUnit
    // DE_N ID:   0 1 2             3           4           5           6           7              8            9               10          11           12                  13

    fseek(f, 0, SEEK_SET);
    static uint16_t endian = 'II';
    fwrite(&endian, 2, 1, f);
    static uint16_t tiff_magic_number = 42;
    fwrite(&tiff_magic_number, 2, 1, f);
    uint32_t ifd_offset = sum + 8; // 8 (IFH size)
    fwrite(&ifd_offset, 4, 1, f);

    // Insert the image data
    fwrite(img.data, 2, h * block_size, f);

    //    fseek(f, ifd_offset, SEEK_SET);
    static uint16_t num_de = 14;
    fwrite(&num_de, 2, 1, f);
    // 256 image width
    static uint16_t tag_w = 256;
    fwrite(&tag_w, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&w, 4, 1, f);

    // 257 image height
    static uint16_t tag_h = 257;
    fwrite(&tag_h, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&h, 4, 1, f);

    // 258 bits per sample
    static uint16_t tag_bits_per_sample = 258;
    fwrite(&tag_bits_per_sample, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    switch (channel) {
    case 1: {
        fwrite(&count_1, 4, 1, f);
        fwrite(&depth, 4, 1, f);
        break;
    }
    case 3: {
        fwrite(&count_3, 4, 1, f);
        // 8 (IFH size) + (2 + 14 * 12 + 4) (0th IFD size: 2->size of the num of DE = 14, 12->size of one DE, 4->size of offset of next IFD) = 182
        uint32_t tag_bits_per_sample_offset = sum + 182;
        fwrite(&tag_bits_per_sample_offset, 4, 1, f);
        break;
    }
    default: break;
    }

    // 259 compression
    static uint16_t tag_compression = 259;
    fwrite(&tag_compression, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&count_1, 4, 1, f); // use 1 for uncompressed

    // 262 photometric interpretation
    static uint16_t tag_photometric_interpretation = 262;
    fwrite(&tag_photometric_interpretation, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(channel == 1 ? &count_1 : &TWO, 4, 1, f); // use 1 for black is zero, use 2 for RGB

    // 273 strip offsets
    static uint16_t tag_strip_offsets = 273;
    fwrite(&tag_strip_offsets, 2, 1, f);
    fwrite(&type_long, 2, 1, f);
    /*  treat every row as a single strip
    fwrite(&h, 4, 1, f);
    // 1 channel
    // 8 (IFH size) + (2 + 14 * 12 + 4) (0th IFD size: 2->size of the num of DE = 14, 12->size of one DE, 4->size of offset of next IFD) = 182
    // 3 channels
    // 182 (bits per sample offsets) + 3 * 2 (3 channels * 2 bytes) = 188
    uint tag_strip_offsets_offset = channel == 1 ? sum + 182 : sum + 188;
    fwrite(&tag_strip_offsets_offset, 4, 1, f);
*/
    fwrite(&count_1, 4, 1, f);
    static uint32_t strip_offset = 8;
    fwrite(&strip_offset, 4, 1, f);

    // 274 orientation
    static uint16_t tag_orientation = 274;
    fwrite(&tag_orientation, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&count_1, 4, 1, f); // 1 = The 0th row represents the visual top of the image, and the 0th column represents the visual left-hand side.

    // 277 samples per pixel
    static uint16_t tag_samples_per_pixel = 277;
    fwrite(&tag_samples_per_pixel, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(channel == 1 ? &count_1 : &count_3, 4, 1, f); // use 1 for grayscale image, use 3 for 3-channel RGB image

    // 278 rows per strip
    static uint16_t tag_rows_per_strip = 278;
    fwrite(&tag_rows_per_strip, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&h, 4, 1, f); // all rows in one strip

    // 279 strip byte counts
    static uint16_t tag_strip_byte_counts = 279;
    fwrite(&tag_strip_byte_counts, 2, 1, f);
    fwrite(&type_long, 2, 1, f);
    /*  treat every row as a single strip
    fwrite(&h, 4, 1, f);
    // 1 channel
    // 182 (offset of strip offsets), 4 * h (length of tag 273, strip_offsets)
    // 3 channels
    // 188 (offset of strip offsets), 4 * h (length of tag 273, strip_offsets)
    uint tag_strip_byte_counts_offset = channel == 1 ? sum + 182 + 4 * h : sum + 188 + 4 * h;
    fwrite(&tag_strip_byte_counts_offset, 4, 1, f);
*/
    fwrite(&count_1, 4, 1, f);
    fwrite(&sum, 4, 1, f);

    // 282 X resolution
    static uint16_t tag_x_resolution = 282;
    fwrite(&tag_x_resolution, 2, 1, f);
    fwrite(&type_fraction, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    /*  treat every row as a single strip
    uint tag_x_resolution_offset = channel == 1 ? sum + 182 + 8 * h : sum + 188 + 8 * h;
    fwrite(&tag_x_resolution_offset, 4, 1, f);
*/
    uint32_t tag_x_resolution_offset = channel == 1 ? sum + 182 : sum + 188;
    fwrite(&tag_x_resolution_offset, 4, 1, f);

    // 283 Y resolution
    static uint16_t tag_y_resolution = 283;
    fwrite(&tag_y_resolution, 2, 1, f);
    fwrite(&type_fraction, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    /*  treat every row as a single strip
    uint tag_y_resolution_offset = channel == 1 ? sum + 190 + 8 * h : sum + 196 + 8 * h;
    fwrite(&tag_y_resolution_offset, 4, 1, f);
*/
    uint32_t tag_y_resolution_offset = channel == 1 ? sum + 190 : sum + 196;
    fwrite(&tag_y_resolution_offset, 4, 1, f);

    // 284 planar configuration
    static uint16_t tag_planar_configuration = 284;
    fwrite(&tag_planar_configuration, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&count_1, 4, 1, f); // use 1 for chunky format

    // 296 resolution unit
    static uint16_t tag_resolution_unit = 296;
    fwrite(&tag_resolution_unit, 2, 1, f);
    fwrite(&type_short, 2, 1, f);
    fwrite(&count_1, 4, 1, f);
    fwrite(&TWO, 4, 1, f); // use 2 for inch

    fwrite(&next_ifd_offset, 4, 1, f);

    // (rgb only) value for tag 258, offset: sum + 182
    if (channel == 3) for(i = 0; i < 3; i++) fwrite(&depth, 2, 1, f);

    /*  treat every row as a single strip
    // value for tag 273, offset: sum + 182 (gray), sum + 188 (rgb)
    uint strip_offset = 8;
    for(i = 0; i < h; i++, strip_offset += block_size) fwrite(&strip_offset, 4, 1, f);

    // value for tag 279, offset: sum + 182 + 4 * h (gray), sum + 188 + 4 * h (rgb)
    for(i = 0; i < h; i++) fwrite(&block_size, 4, 1, f);
*/

    // value for tag 282, offset: sum + 182 + 8 * h (gray), sum + 188 + 8 * h (rgb)
    static uint32_t res_physical = 1, res_pixel = 72;
    fwrite(&res_pixel, 4, 1, f); // fraction w-pixel
    fwrite(&res_physical, 4, 1, f); // denominator PhyWidth-inch

    // value for tag 283, offset: sum + 190 + 8 * h (gray), sum + 196 + 8 * h (rgb)
    fwrite(&res_pixel, 4, 1, f); // fraction h-pixel
    fwrite(&res_physical,4, 1, f); // denominator PhyHeight-inch

    fclose(f);
}

bool ImageIO::load_image_tif(cv::Mat &img, QString filename)
{
    // TODO test motorola tiff read function
    static uint16_t byte_order;
    static bool is_big_endian;
    static uint32_t ifd_offset;
    static uint16_t num_de;
    static uint16_t tag_number, tag_data_type;
    static uint32_t tag_data_num, tag_data;
    static int width, height, depth;
    FILE *f = fopen(filename.toLocal8Bit().constData(), "rb");

    // tiff header
    fseek(f, 0, SEEK_SET);
    fread(&byte_order, 2, 1, f);
    is_big_endian = byte_order == 'MM';
    fread(&ifd_offset, 2, 1, f); // magic number 42
    fread(&ifd_offset, 4, 1, f);
    if (is_big_endian) ifd_offset = qFromBigEndian(ifd_offset);

    // skip image data before fetching image info
    fseek(f, ifd_offset, SEEK_SET);
    //    qDebug() << "ifd_offset " << ifd_offset;

    // IFH data
    fread(&num_de, 2, 1, f);
    if (is_big_endian) num_de = qFromBigEndian(num_de);
    for (uint16_t c = 0; c < num_de; c++) {
        // get tag #
        fread(&tag_number, 2, 1, f);
        if (is_big_endian) tag_number = qFromBigEndian(tag_number);
        // get tag type
        fread(&tag_data_type, 2, 1, f);
        if (is_big_endian) tag_data_type = qFromBigEndian(tag_data_type);
        // get tag data num
        fread(&tag_data_num, 4, 1, f);
        if (is_big_endian) tag_data_num = qFromBigEndian(tag_data_num);
        // get tag data (or ptr offset)
        fread(&tag_data, 4, 1, f);
        if (is_big_endian) tag_data = qFromBigEndian(tag_data) / (tag_data_type == 3 ? 1 << 16 : 1);

        //        qDebug() << tag_number << tag_data_type << tag_data_num << hex << tag_data;
        switch (tag_number) {
        case 256: width = tag_data; break;
        case 257: height = tag_data; break;
        case 258:
            if (tag_data_num != 1) return -1;
            depth = tag_data;
            break;
        case 259:
            if (tag_data != 1) return -1;
            break;
        case 279:
            if (width * height * depth / 8 != tag_data) return -1;
            break;
        default: break;
        }
    }

    img = cv::Mat(height, width, depth == 8 ? CV_8U : CV_16U);
    fseek(f, 8, SEEK_SET);
    fread(img.data, 2, height * width, f);
    if (is_big_endian) img = (img & 0xFF00) / (1 << 8) + (img & 0x00FF) * (1 << 8);

    return 0;
}
