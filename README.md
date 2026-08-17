# Ücretsiz Asas, Warrior, BP Makrosu.
C++ Makro Projesi

Projeyi kullanıma açıyorum, dilediğiniz gibi kullanabilirsiniz.

Proje temelinde Asas makrosu olarak geliştirildi, yani varsayılan ayarları Asas'a göre ayarlıdır. 

----------Macro Performans Önerileri/Rehberi--------

Diğer joblar içinde ms düzeyini 350-650 arası değiştirerek stabil bir şekilde skill kaçırmadan kullanabilirsiniz.

Atta yer alan Skill basma, Skill Bekleme, R basma, R bekleme süreleriyle oynamanızı tavsiye etmem. 
Üstte yer alan Hız, genel olarak makronun skill vurma döngüsünü kontrol eder. Bu bütün joblarda 350-650 arası ideal hızlarda neredeyse hiç beklemeden takır takır vurmasını sağlar.
Aşağıdaki "Skill Bekleme" süresi ise, 1 skilli vurduktan sonra diğer skilleri, ne kadar süre bekleyip vuracağını belirtir. 

Warriorlarda yere vurma, BP'de 72 öncesi mana az harcasın diye koyduğunuz diğer atak skilleri, asasta kullandığınız atak skillerinin arasındaki bekleme süresini, yani skill vurduktan sonra diğerini ne kadar hızlı vuracağınızı bu kısım "Skill Bekleme" belirler. 

Bu kısım çok önemlidir. Çünkü sunucu ping süresi, lag durumu, fps değerleriniz, hangi haritada olduğunuz, buradaki performansınızı çok etkiler. CZ için ideal değer 5'tir. Ping veya düşük fps durumlarında sekme yaşanırsa, Skill Beklemeyi 5'in katları olacak şekilde en fazla 25'e, Hız ayarını ise en fazla 1250'ye getirmeniz önerilir. İdeal oranlar en fazla 15-950'dir. Fazlası sizin bu saydığım koşullara göre sizin bileceğiniz iş. 

Genel durum iyiyse, genellikle Usko ve bütün pvp'lerde 5-350 fazlasıyla yeterli olacaktır. 

----------- Kişisel kullanım için zevkine geliştirildi, aylık 850 lira gibi saçma sapan fiyatlar vermeyin diye bütün projeyi, derlenmiş haliyle veriyorum. Meraklısı daha da geliştirebilir. ------------

Projeye OpenCV entegre edilmiştir. Geliştirmeye devam ederseniz eğer, görüntü okumayla sınırsız özellikler ekleyebilirsiniz.

-------- HP, MP, Minör kısayollarını, MacroEngine.cpp dosyası içinde ilgili modüllerden, en alt satırlardaki methodlar, dilediğiniz kısayol tuşlarını atayarak değiştirebilirsiniz. Ben şu an için daha kolay oluyor diye HP pot'u F tuşuna, Minör tuşunu mouse yan tuş 1'e, MP potu'da mouse yan tuş 2'ye atadım. Klavyem Logitech g120. Size rehber olsun.---------

-------- Pro Asas Akıllı modunda ilgili kısımlara ilgili skilleri ekleyin. Bu modun olayı, bütün skillerin bekleme sürelerine göre otomatik vurmasıdır. Tuş spamlamaz. F1 ve F2'deki skillerin her birinin bekleme süresi kendisine göredir. F1'deki skilleri vurduktan sonra makro öylece bekler. Bekleme süresi dolana kadar. Size düşen bir sayfayı vurduktan sonra diğerine geçmektir. Sakin PVE modunda gayet güzel çalışır, PVP için uzmanlık gerektirir. Slot kavgaları için önermem. Skill takip etmeniz gerekeceği için verim düşer. Slot kavgalarında Klasik modu kullanın. ----------------

