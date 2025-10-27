namespace SecureChat.Data
{
    // ─────────────────────────────────────────────────────────────────────────────
    // ENUM'LAR
    // ─────────────────────────────────────────────────────────────────────────────

    /// <summary>
    /// Bir konuşmanın türü:
    /// - Direct   : iki kullanıcı arası bire bir
    /// - Group    : birden çok kullanıcı
    /// - Broadcast: "tüm kullanıcılara" (fan-out) gönderimler için mantıksal kanal
    /// </summary>
    public enum ConversationType { Direct, Group, Broadcast }

    /// <summary>
    /// Grup/direkt konuşmada katılımcı rolü. Yalnızca yetki seviyesini belirler;
    /// E2EE anahtar kurulumuna etkisi yoktur, o cihaz/kullanıcı bazlıdır.
    /// </summary>
    public enum ParticipantRole { Owner, Admin, Member }

    /// <summary>
    /// Bir mesajın alıcı cihaza doğru teslimat durumu.
    /// queued    : Sunucuda kuyruğa alındı (alıcı çevrimdışı olabilir).
    /// sent      : Soket/push ile gönderim denendi (ağ katmanına iletildi).
    /// delivered : İstemci tarafı "teslim alındı" sinyali verdi.
    /// ack       : Kriptografik ACK (örn. protokol seviyesinde) veya app ACK.
    /// read      : Kullanıcı mesajı görüntüledi (okundu bilgisi).
    /// failed    : Gönderim artık denenmeyecek (kalıcı hata).
    /// </summary>
    public enum DeliveryStatus { Queued, Sent, Delivered, Ack, Read, Failed }

    // ─────────────────────────────────────────────────────────────────────────────
    // USERS
    // ─────────────────────────────────────────────────────────────────────────────

    public class User
    {
        /// <summary>
        /// Kullanıcının benzersiz kimliği (UUID). Veritabanı birincil anahtarı.
        /// Uygulama genelinde referans olarak kullanılır.
        /// </summary>
        public Guid Id { get; set; }

        /// <summary>
        /// Giriş/mention için benzersiz kullanıcı adı.
        /// PostgreSQL' de "citext" (büyük/küçük harfe duyarsız) tutulur.
        /// </summary>
        public string Username { get; set; } = default!;

        /// <summary>
        /// Sunucu tarafında yapılan kimlik doğrulama için parola özeti.
        /// Not: E2EE'yi etkilemez; mesaj içeriklerini sunucu asla plaintext görmez.
        /// </summary>
        public string? PasswordHash { get; set; }

        /// <summary>
        /// Hesap durumu (örn. "active", "suspended"). Yetkilendirme ve moderasyon için.
        /// </summary>
        public string Status { get; set; } = "Online";

        /// <summary>
        /// Kullanıcı kaydının oluşturulduğu zaman (UTC).
        /// </summary>
        public DateTimeOffset CreatedAt { get; set; }

        // Navigations
        public ICollection<UserDevice> Devices { get; set; } = new List<UserDevice>();
        public ICollection<ConversationParticipant> ConversationParticipants { get; set; } = new List<ConversationParticipant>();
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // USER_DEVICES (çok cihaz desteği + E2EE'nin güven kökü cihazdır)
    // ─────────────────────────────────────────────────────────────────────────────

    public class UserDevice
    {
        /// <summary>
        /// Cihazın benzersiz kimliği (UUID). Her cihaz için ayrı E2EE kimlik anahtarı tutulur.
        /// </summary>
        public Guid Id { get; set; }

        /// <summary>
        /// Bu cihazın sahibi olan kullanıcı (FK → users).
        /// Kullanıcı silinirse cihazlar da silinir (CASCADE).
        /// </summary>
        public Guid UserId { get; set; }

        /// <summary>
        /// "iPhone 15", "Work Laptop" gibi etiket; kullanıcıya cihazı tanımlamak için.
        /// </summary>
        public string? DeviceLabel { get; set; }

        /// <summary>
        /// Cihaz kaydının oluşturulduğu zaman.
        /// </summary>
        public DateTimeOffset CreatedAt { get; set; }

        /// <summary>
        /// Sunucunun cihazı en son gördüğü zaman (presence/çevrimiçi göstergesi).
        /// </summary>
        public DateTimeOffset? LastSeenAt { get; set; }

        /// <summary>
        /// Son görülen IP (güvenlik, denetim ve anomali tespitinde yararlı).
        /// PostgreSQL’de "inet" kolon tipine map edilebilir.
        /// </summary>
        public string? IpLast { get; set; }

        // Navigations
        public User User { get; set; } = default!;
        public DeviceIdentityKey? IdentityKey { get; set; }
        public ICollection<DeviceSignedPrekey> SignedPrekeys { get; set; } = new List<DeviceSignedPrekey>();
        public ICollection<DeviceOneTimePrekey> OneTimePrekeys { get; set; } = new List<DeviceOneTimePrekey>();
        public ICollection<Message> MessagesSent { get; set; } = new List<Message>();
        public ICollection<MessageDelivery> DeliveriesReceived { get; set; } = new List<MessageDelivery>();
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // CONVERSATIONS (Direct / Group / Broadcast)
    // ─────────────────────────────────────────────────────────────────────────────

    public class Conversation
    {
        /// <summary>
        /// Sohbet kanalı kimliği (UUID). Bir kanalda birden çok mesaj bulunur.
        /// </summary>
        public Guid Id { get; set; }

        /// <summary>
        /// Kanalın türü: bire bir, grup veya broadcast (tüm kullanıcılara).
        /// a/b/c gereksinimini bu alanla ayırt ederiz.
        /// </summary>
        public ConversationType Type { get; set; }

        /// <summary>
        /// Grup veya broadcast için görünen başlık. Direct sohbetlerde boş olabilir.
        /// </summary>
        public string? Title { get; set; }

        /// <summary>
        /// Kanali oluşturan kullanıcı (FK → users). NULL olabilir (sistem kanalı gibi).
        /// </summary>
        public Guid? CreatedByUserId { get; set; }

        /// <summary>
        /// Kanalın oluşturulma zamanı.
        /// </summary>
        public DateTimeOffset CreatedAt { get; set; }

        // Navigations
        public User? CreatedByUser { get; set; }
        public ICollection<ConversationParticipant> Participants { get; set; } = new List<ConversationParticipant>();
        public ICollection<Message> Messages { get; set; } = new List<Message>();
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // CONVERSATION_PARTICIPANTS (üyelikler)
    // ─────────────────────────────────────────────────────────────────────────────

    public class ConversationParticipant
    {
        /// <summary>
        /// Üyeliğin ait olduğu konuşma (Composite PK'nin bir parçası).
        /// </summary>
        public Guid ConversationId { get; set; }

        /// <summary>
        /// Konuşmaya üye kullanıcı (Composite PK'nin bir parçası).
        /// </summary>
        public Guid UserId { get; set; }

        /// <summary>
        /// Kullanıcının rolu: owner/admin/member. Moderasyon ve yönetim yetkileri için.
        /// </summary>
        public ParticipantRole Role { get; set; } = ParticipantRole.Member;

        /// <summary>
        /// Kullanıcının kanala katıldığı zaman (denetim izi).
        /// </summary>
        public DateTimeOffset JoinedAt { get; set; }

        /// <summary>
        /// Kullanıcı kanaldan ayrıldıysa tarih; NULL ise üyelik aktiftir.
        /// </summary>
        public DateTimeOffset? LeftAt { get; set; }

        // Navigations
        public Conversation Conversation { get; set; } = default!;
        public User User { get; set; } = default!;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // MESSAGES (ŞİFRELİ içerik + hash)
    // ─────────────────────────────────────────────────────────────────────────────

    public class Message
    {
        /// <summary>
        /// Mesaj kimliği (UUID). ULID/UUIDv7 de kullanılabilir; zaman sıralaması kolaylaşır.
        /// </summary>
        public Guid Id { get; set; }

        /// <summary>
        /// Mesajın ait olduğu konuşma (FK → conversations).
        /// Silinirse mesajlar da silinir (CASCADE).
        /// </summary>
        public Guid ConversationId { get; set; }

        /// <summary>
        /// Mesajı gönderen cihaz (FK → user_devices).
        /// Çok cihazlı E2EE'de her cihazın ayrı "ratchet" durumu olduğundan kritiktir.
        /// </summary>
        public Guid SenderDeviceId { get; set; }

        /// <summary>
        /// Şifreli mesaj içeriği (ciphertext). Sunucu plaintext'i asla saklamaz/görmez.
        /// </summary>
        public byte[] Ciphertext { get; set; } = default!;

        /// <summary>
        /// E2EE protokol üstbilgisi: ör. Signal/Double Ratchet header, nonce, versiyon,
        /// AEAD "associated data" ipuçları vb. JSONB olarak saklanır.
        /// </summary>
        public string? ProtocolHeader { get; set; }

        /// <summary>
        /// Ciphertext'in SHA-256 (veya seçtiğiniz algoritma) hex özeti.
        /// Şarttaki "mesajların hashleri alınarak kaydedilmelidir" gereksinimini sağlar.
        /// Bütünlük ve denetim için kullanılır; plaintext hash'i sunucuda tutulmaz.
        /// </summary>
        public string CiphertextHashSha256 { get; set; } = default!;

        /// <summary>
        /// Mesajın oluşturulma zamanı (sıralama, partisyonlama ve TTL işlemleri için).
        /// </summary>
        public DateTimeOffset CreatedAt { get; set; }

        // Navigations
        public Conversation Conversation { get; set; } = default!;
        public UserDevice SenderDevice { get; set; } = default!;
        public ICollection<MessageDelivery> Deliveries { get; set; } = new List<MessageDelivery>();
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // MESSAGE_DELIVERIES (çevrimdışı kuyruk + cihaz bazlı teslimat durumları)
    // ─────────────────────────────────────────────────────────────────────────────

    public class MessageDelivery
    {
        /// <summary>
        /// Teslimat kaydının kimliği (UUID). Her (mesaj, alıcı-cihaz) çifti için tekil kayıt.
        /// </summary>
        public Guid Id { get; set; }

        /// <summary>
        /// Hangi mesaja ait olduğu (FK → messages). Mesaj silinirse teslimat kayıtları da silinir.
        /// </summary>
        public Guid MessageId { get; set; }

        /// <summary>
        /// Hedef alıcı cihaz (FK → user_devices). Bir kullanıcının birden çok cihazı olabilir;
        /// her cihaz için ayrı teslimat kaydı tutmak E2EE ile uyumludur.
        /// </summary>
        public Guid RecipientDeviceId { get; set; }

        /// <summary>
        /// Teslimatın mevcut durumu (queued/sent/delivered/ack/read/failed).
        /// İstemci ACK/Read event'leri geldiğinde güncellenir.
        /// </summary>
        public DeliveryStatus Status { get; set; } = DeliveryStatus.Queued;

        /// <summary>
        /// Kaydın kuyruğa alındığı zaman (sunucuya ilk yazıldığı an).
        /// </summary>
        public DateTimeOffset QueuedAt { get; set; }

        /// <summary>
        /// Soket/push ile gönderim denemesinin zaman damgası.
        /// </summary>
        public DateTimeOffset? SentAt { get; set; }

        /// <summary>
        /// İstemci mesajı aldığını bildirdiği an (delivery receipt).
        /// </summary>
        public DateTimeOffset? DeliveredAt { get; set; }

        /// <summary>
        /// Kullanıcı mesajı açtığında/okuduğunda gelen sinyalin zamanı (read receipt).
        /// </summary>
        public DateTimeOffset? ReadAt { get; set; }

        /// <summary>
        /// Kalıcı veya geçici hata nedenlerini kaydetmek için serbest metin (örn. "invalid token").
        /// </summary>
        public string? FailReason { get; set; }

        /// <summary>
        /// Yeniden deneme planlaması için hedef zaman (örn. exponential backoff).
        /// </summary>
        public DateTimeOffset? NextRetryAt { get; set; }

        // Navigations
        public Message Message { get; set; } = default!;
        public UserDevice RecipientDevice { get; set; } = default!;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // E2EE ANAHTAR TABLOLARI (cihaz başına)
    // ─────────────────────────────────────────────────────────────────────────────

    /// <summary>
    /// Cihazın uzun ömürlü kimlik açık anahtarı (Ed25519/X25519 vb.).
    /// Güven kökü olarak kullanılır; signed prekey'ler bu anahtarla imzalanır.
    /// </summary>
    public class DeviceIdentityKey
    {
        /// <summary>
        /// Aynı zamanda birincil anahtar. Her cihaz için tam 1 kimlik anahtarı kaydı.
        /// Cihaz silinirse anahtar kaydı da silinir (CASCADE).
        /// </summary>
        public Guid DeviceId { get; set; }

        /// <summary>
        /// Cihazın açık anahtarı (public). Gizli anahtar istemcide kalır, sunucu görmez.
        /// </summary>
        public byte[] PublicIdentityKey { get; set; } = default!;

        /// <summary>
        /// Anahtarın oluşturulup sunucuya yüklendiği zaman.
        /// </summary>
        public DateTimeOffset CreatedAt { get; set; }

        public UserDevice Device { get; set; } = default!;
    }

    /// <summary>
    /// İmzalı prekey: Oturum başlatırken kimlik anahtarı tarafından imzalanmış orta-ömürlü anahtar.
    /// İlk eşleşme ve anahtar yenilemeleri için kullanılır.
    /// </summary>
    public class DeviceSignedPrekey
    {
        /// <summary>
        /// Kayıt kimliği (UUID).
        /// </summary>
        public Guid Id { get; set; }

        /// <summary>
        /// Hangi cihaza ait olduğu (FK → user_devices).
        /// </summary>
        public Guid DeviceId { get; set; }

        /// <summary>
        /// İmzalanan açık anahtar değeri (public prekey).
        /// </summary>
        public byte[] PublicPrekey { get; set; } = default!;

        /// <summary>
        /// PublicPrekey üzerinde kimlik anahtarı ile üretilmiş dijital imza.
        /// Alıcı cihaz, bu imzayı doğrulayarak araya girme saldırılarını tespit eder.
        /// </summary>
        public byte[] Signature { get; set; } = default!;

        /// <summary>
        /// Prekey'in yayımlandığı zaman.
        /// </summary>
        public DateTimeOffset CreatedAt { get; set; }

        /// <summary>
        /// Prekey son kullanma tarihi (rotasyon planı için).
        /// </summary>
        public DateTimeOffset? ExpiresAt { get; set; }

        public UserDevice Device { get; set; } = default!;
    }

    /// <summary>
    /// Tek kullanımlık prekey: Her ilk temas için bir kez tüketilir; yeniden kullanılmaz.
    /// Kimlik ve gizliliği korumak için havuz halinde sunucuda saklanır.
    /// </summary>
    public class DeviceOneTimePrekey
    {
        /// <summary>
        /// Kayıt kimliği (UUID).
        /// </summary>
        public Guid Id { get; set; }

        /// <summary>
        /// Hangi cihaza ait olduğu (FK → user_devices).
        /// </summary>
        public Guid DeviceId { get; set; }

        /// <summary>
        /// Tek seferlik açık anahtar (public).
        /// </summary>
        public byte[] PublicPrekey { get; set; } = default!;

        /// <summary>
        /// Sunucuya yüklenme zamanı (havuzda kullanılabilir hâle geliş).
        /// </summary>
        public DateTimeOffset PublishedAt { get; set; }

        /// <summary>
        /// Bu prekey ilk eşleşmede kullanıldığında işaretlenir; tekrar tahsis edilmez.
        /// </summary>
        public DateTimeOffset? ConsumedAt { get; set; }

        public UserDevice Device { get; set; } = default!;
    }
}